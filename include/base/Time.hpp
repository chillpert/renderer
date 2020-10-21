#ifndef TIME_HPP
#define TIME_HPP

#include "Core.hpp"

namespace RAYEX_NAMESPACE
{
  class Window;

  /// Used to keep track of the application's timing.
  /// @ingroup BASE
  /// @todo Average FPS are pointless. Implement minimum FPS and frametimes instead.
  class RX_API Time
  {
  public:
    friend Window;

    /// Prints the average fps as well as the time since recording.
    static void benchmark( );

    /// @return Returns the time passed since application start in seconds.
    static auto getTime( ) -> float;
    /// @return Returns the time passed between the current and the last frame.
    static auto getDeltaTime( ) -> float;

  private:
    /// Updates the timing.
    ///
    /// Prints the current fps every three seconds.
    /// @note This function will be called every tick.
    static void update( );
  };
} // namespace RAYEX_NAMESPACE

#endif // TIME_HPP
