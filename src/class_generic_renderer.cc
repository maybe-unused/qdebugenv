#include <qdebugenv/class_generic_renderer.h>

#include <qguiapplication.h>
#include <qquickwindow.h>
#include <private/qquickgraphicsinfo_p.h>
#include <floppy/logging.h>
#include <qdebugenv/rhi/class_renderer.h>
#include <qdebugenv/rhi/class_immediate_gui_bridge.h>
#include <qdebugenv/vendored/imgui.h>

#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
# include <qsgrendernode.h>
#else // QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
# include <private/qsgrendernode_p.h>
#endif // QT_VERSION > QT_VERSION_CHECK(6, 6, 0)

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
# define QDE_WELL_BEHAVING_DEPTH 1
#endif // QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)

namespace llog = ::floppy::log;
namespace
{
  using namespace ::qdebugenv;
  class CRhiImguiNode : public ::QSGRenderNode
  {
   public:
    CRhiImguiNode(::QQuickWindow* window, CGenericRenderer* item)
      : window(window)
      , item(item)
      , renderer(new CRhiRenderer)
      , custom_renderer(this->item->create_custom_renderer())
    {}

    ~CRhiImguiNode() {
      delete this->renderer;
      delete this->custom_renderer;
    }

    virtual auto prepare() -> void override {
      #if QT_VERSION_MAJOR > 6 || QT_VERSION_MINOR >= 6
      auto* rhi = this->window->rhi();
      #else // QT_VERSION_MAJOR > 6 || QT_VERSION_MINOR >= 6
      auto* rif = this->window->rendererInterface();
      auto* rhi = static_cast<::QRhi*>(rif->getResource(this->window, ::QSGRendererInterface::RhiResouce));
      #endif // QT_VERSION_MAJOR > 6 || QT_VERSION_MINOR >= 6
      if(not rhi) {
        llog::error("CRhiImguiNode::prepare: no rhi found for window {}", static_cast<void*>(this->window));
        return;
      }
      if(this->custom_renderer)
        this->custom_renderer->render();

      #if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
      auto* rt = this->renderTarget();
      auto* cb = this->commandBuffer();
      #else // QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
      auto* d = ::QSGRenderNodePrivate::get(this);
      auto* rt = d->m_rt.rt;
      auto* cb = d->m_rt.cb;
      #endif // QT_VERSION > QT_VERSION_CHECK(6, 6, 0)

      #if defined(QDE_WELL_BEHAVING_DEPTH)
      auto const mvp = *this->projectionMatrix() * *this->matrix();
      #else // defined(QDE_WELL_BEHAVING_DEPTH)
      auto mvp = rhi->clipSpaceCorrMatrix();
      auto const output_size = rt->pixelSize();
      auto const dpr = rt->devicePixelRatio();
      mvp.ortho(0, output_size.width() / dpr, output_size.height() / dpr, 0, -1, 1);
      mvp *= *this->matrix();
      #endif // defined(QDE_WELL_BEHAVING_DEPTH)

      auto const opacity = static_cast<f32>(this->inheritedOpacity());
      this->renderer->prepare(rhi, rt, cb, mvp, opacity);
    }

    virtual auto render([[maybe_unused]] ::QSGRenderNode::RenderState const* state) -> void override {
      this->renderer->render();
    }

    virtual auto releaseResources() -> void override {
      this->renderer->release_resources();
    }

    [[nodiscard]] virtual auto changedStates() const -> ::QSGRenderNode::StateFlags override {
      return DepthState | ScissorState | ColorState | BlendState | CullState | ViewportState;
    }

    [[nodiscard]] virtual auto flags() const -> ::QSGRenderNode::RenderingFlags override {
      auto result = ::QSGRenderNode::RenderingFlags(NoExternalRendering);

      #if defined(QDE_WELL_BEHAVING_DEPTH)
      result |= DepthAwareRendering;
      #endif // defined(QDE_WELL_BEHAVING_DEPTH)

      return result;
    }

    ::QQuickWindow* window;
    CGenericRenderer* item;
    CRhiRenderer* renderer;
    CRhiImmediateGuiCustomRenderer* custom_renderer = nullptr;
  };
} // namespace

namespace qdebugenv
{
  CRhiImmediateGuiCustomRenderer::~CRhiImmediateGuiCustomRenderer() = default;
  auto CRhiImmediateGuiCustomRenderer::sync([[maybe_unused]] CRhiRenderer* renderer) -> void {}
  auto CRhiImmediateGuiCustomRenderer::render() -> void {}

  struct CGenericRenderer::impl
  {
    CGenericRenderer* q;
    ::QQuickWindow* window = nullptr;
    ::QMetaObject::Connection window_connection;
    CImmediateGuiBridge gui;
    ::QQuickGraphicsInfo* graphics_info = nullptr;
    bool show_metrics = true;

    explicit impl(CGenericRenderer* q) : q(q) {}
  };

  CGenericRenderer::CGenericRenderer(::QQuickItem* parent)
    : QQuickItem(parent)
    , impl_(fl::make_box<impl>(this)) {
    this->setFlag(::QQuickItem::ItemHasContents, true);
    this->setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
    this->setAcceptHoverEvents(true);
  }

  CGenericRenderer::~CGenericRenderer() = default;

  auto CGenericRenderer::frame() -> void {
    auto& io = ImGui::GetIO();
    ImGui::Begin("Scenegraph metrics", &this->impl_->show_metrics);
    if(this->impl_->graphics_info) {
//      ImGui::Text("Backend: %s", this->impl_->graphics_info->api());
//      ImGui::Separator();
//      ImGui::Text("Version: %d.%d", this->impl_->graphics_info->majorVersion(), this->impl_->graphics_info->minorVersion());
//      ImGui::Text("Profile: %s", this->impl_->graphics_info->profile());
//      ImGui::Text("Renderable type: %s", this->impl_->graphics_info->renderableType());
//      ImGui::Text("Shader compilation type: %s", this->impl_->graphics_info->shaderCompilationType());
//      ImGui::Text("Shader source type: %s", this->impl_->graphics_info->shaderSourceType());
//      ImGui::Text("Shader type: %s", this->impl_->graphics_info->shaderType());
    } else
      ImGui::Text("No graphics info available");
    ImGui::Separator();
    ImGui::Text("Framerate: %.1f (%.5f ms/frame)", io.Framerate, 1'000.0F / io.Framerate);
    ImGui::End();
  }

  auto CGenericRenderer::create_custom_renderer() -> CRhiImmediateGuiCustomRenderer* {
    return nullptr;
  }

  auto CGenericRenderer::bridge() -> CImmediateGuiBridge* {
    return &this->impl_->gui;
  }

  auto CGenericRenderer::graphics_info() const -> QObject* {
    return this->impl_->graphics_info;
  }

  auto CGenericRenderer::set_graphics_info(QObject* info) -> void {
    if(this->impl_->graphics_info == info)
      return;
    //this->impl_->graphics_info = qobject_cast<::QQuickGraphicsInfo*>(info);
    emit this->impl_->q->graphics_info_changed();
  }

  auto CGenericRenderer::updatePaintNode(
    ::QSGNode* node,
    [[maybe_unused]] ::QQuickItem::UpdatePaintNodeData* data
  ) -> ::QSGNode* {
    if(this->size().isEmpty()) {
      delete node; // NOLINT(*-owning-memory)
      return nullptr;
    }
    auto* n = dynamic_cast<CRhiImguiNode*>(node);
    if(not n)
      n = new CRhiImguiNode(this->impl_->window, this); // NOLINT(*-owning-memory)
    this->impl_->gui.sync_renderer(n->renderer);
    if(n->custom_renderer)
      n->custom_renderer->sync(n->renderer);
    n->markDirty(::QSGNode::DirtyMaterial);
    return n;
  }
  auto CGenericRenderer::itemChange(
    ::QQuickItem::ItemChange change,
    ::QQuickItem::ItemChangeData const& value) -> void {
    if(change != ::QQuickItem::ItemSceneChange)
      return;
    if(this->impl_->window) {
      ::QObject::disconnect(this->impl_->window_connection);
      this->impl_->window = nullptr;
    }
    if(value.window) { // NOLINT(*-pro-type-union-access)
      this->impl_->window = this->window();
      this->impl_->window_connection = QObject::connect(
        this->impl_->window,
        &::QQuickWindow::afterAnimating,
        this->impl_->window,
        [this] -> void {
          if(not this->isVisible())
            return;
          this->impl_->gui.next_frame(
            this->size(),
            static_cast<f32>(this->impl_->window->effectiveDevicePixelRatio()),
            this->mapToScene({0., 0.}),
            [this] -> void { this->frame(); }
          );
          this->update();
          if(not this->impl_->window->isSceneGraphInitialized())
            this->impl_->window->update();
        });
    }
  }
  auto CGenericRenderer::keyPressEvent(::QKeyEvent* event) -> void { this->impl_->gui.process_event(event); }
  auto CGenericRenderer::keyReleaseEvent(::QKeyEvent* event) -> void { this->impl_->gui.process_event(event); }
  auto CGenericRenderer::mousePressEvent(::QMouseEvent* event) -> void {
    this->forceActiveFocus(Qt::MouseFocusReason);
    this->impl_->gui.process_event(event);
  }
  auto CGenericRenderer::mouseReleaseEvent(::QMouseEvent* event) -> void { this->impl_->gui.process_event(event); }
  auto CGenericRenderer::mouseMoveEvent(::QMouseEvent* event) -> void { this->impl_->gui.process_event(event); }
  auto CGenericRenderer::mouseDoubleClickEvent(::QMouseEvent* event) -> void { this->impl_->gui.process_event(event); }
  auto CGenericRenderer::wheelEvent(::QWheelEvent* event) -> void { this->impl_->gui.process_event(event); }
  auto CGenericRenderer::hoverMoveEvent(::QHoverEvent* event) -> void {
    if(::QGuiApplication::mouseButtons() != Qt::NoButton)
      return;
    auto const scene_offset = this->mapToScene(event->position());
    auto const global_offset = this->mapToGlobal(scene_offset);
    auto e = ::QMouseEvent(
      ::QEvent::MouseMove,
      event->position(),
      event->position() + scene_offset,
      event->position() + global_offset,
      Qt::NoButton,
      Qt::NoButton,
      ::QGuiApplication::keyboardModifiers()
    );
    this->impl_->gui.process_event(&e);
  }
  auto CGenericRenderer::touchEvent(::QTouchEvent* event) -> void { this->impl_->gui.process_event(event); }
} // namespace qdebugenv