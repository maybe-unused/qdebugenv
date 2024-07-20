#include <qdebugenv/class_extendable_renderer.h>

#include <qqml.h>

namespace qdebugenv
{
  namespace qml
  {
    [[maybe_unused]] volatile auto register_imgui_ = []() { // NOLINT(*-identifier-naming)
      fmt::println("<qdebugenv> registering {} qml types", meta::qml_namespace_rendering);
      qmlRegisterModule(meta::qml_namespace_rendering, 1, 0);
      qmlRegisterType<CExtendableRenderer>(meta::qml_namespace_rendering, 1, 0, "ExtendableRenderer");
      //qmlRegisterType(QUrl("qrc:/qml/immediate/Renderer.qml"), "io.qdebugenv.rendering", 1, 0, "ImmediateGUIRenderFacility");
      return true;
    }();
  } // namespace qml

  CExtendableRenderer::CExtendableRenderer(CGenericRenderer* parent)
    : CGenericRenderer(parent)
  {}
  CExtendableRenderer::~CExtendableRenderer() = default;

  auto CExtendableRenderer::frame() -> void {
    this->CGenericRenderer::frame();
    for(auto& callback : this->callbacks_)
      callback();
    for(auto& drawable : this->drawables_)
      drawable->frame();
  }
} // namespace qdebugenv