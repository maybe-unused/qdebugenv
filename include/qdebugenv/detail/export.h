#pragma once

#include <floppy/floppy.h>

#if defined(FLOPPY_OS_WINDOWS)
# if defined(QDE_LIBRARY)
#   define QDE_API __declspec(dllexport)
# elif defined(QDE_STATIC_LIBRARY)
#   define QDE_API
# else
#   define QDE_API __declspec(dllimport)
# endif
#else
# define QDE_API __attribute__((visibility("default")))
#endif

/// \brief Qt Quick Debug Environment library namespace.
namespace qdebugenv
{
  using namespace fl::types;
  using namespace fl::literals;

  /// \brief Namespace for project metadata.
  namespace meta
  {
    /// \brief Meta information about the library.
    [[maybe_unused]] constexpr inline auto qde_meta = fl::meta::project_meta(
      fl::meta::version(
        QDE_PROJECT_VERSION_MAJOR,
        QDE_PROJECT_VERSION_MINOR,
        QDE_PROJECT_VERSION_PATCH
        ),
      std::string_view(QDE_TARGET_NAME),
      "io.github.whs31.qdebugenv",
      "whs31"
    );
  } // namespace meta
} // namespace qdebugenv