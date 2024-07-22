#include <qdebugenv/class_extendable_renderer.h>

#include <qqmlregistration.h>

namespace qdebugenv
{
  namespace qml
  {
    [[maybe_unused]] volatile auto register_1_ = []() { // NOLINT(*-identifier-naming)
      ::qmlRegisterModule(meta::qml_namespace_rendering, 1, 0);
      auto const result
        = ::qmlRegisterType<CGenericRenderer>(meta::qml_namespace_rendering, 1, 0, "GenericRenderer")
        * ::qmlRegisterType<CExtendableRenderer>(meta::qml_namespace_rendering, 1, 0, "ExtendableRenderer")
        * ::qmlRegisterType(QUrl("qrc:/qml/ImmediateGUIRenderingFacility.qml"), "io.qdebugenv.rendering", 1, 0, "ImmediateGUIRenderingFacility")
        <= 0;
      if(not result)
        fl::panic("failed to register qml types (io.qdebugenv.rendering)");
      return result;
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