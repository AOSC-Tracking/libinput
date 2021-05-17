.. _tapping:

==============================================================================
Tap-to-click behaviour
==============================================================================

"Tapping" or "tap-to-click" is the name given to the behavior where a short
finger touch down/up sequence maps into a button click. This is most
commonly used on touchpads, but may be available on other devices.

libinput implements tapping for one, two, and three fingers, where supported
by the hardware, and maps those taps into a left, right, and middle button
click, respectively. Not all devices support three fingers, libinput will
support tapping up to whatever is supported by the hardware. libinput does
not support four-finger taps or any tapping with more than four fingers,
even though some hardware can distinguish between that many fingers.

.. _tapping_default:

------------------------------------------------------------------------------
Tap-to-click default setting
------------------------------------------------------------------------------

Tapping is **disabled** by default on most devices, see
:commit:`2219c12c3` because:

- if you don't know that tapping is a thing (or enabled by default), you get
  spurious button events that make the desktop feel buggy.
- if you do know what tapping is and you want it, you usually know where to
  enable it, or at least you can search for it.

Tapping is **enabled** by default on devices where tapping is the only
method to trigger button clicks. This includes devices without physical
buttons such as touch-capable graphics tablets.

Tapping can be enabled/disabled on a per-device basis. See
**libinput_device_config_tap_set_enabled()** for details.

.. _tapndrag:

------------------------------------------------------------------------------
Tap-and-drag
------------------------------------------------------------------------------

libinput also supports "tap-and-drag" where a tap immediately followed by a
finger down and that finger being held down emulates a button press. Moving
the finger around can thus drag the selected item on the screen.
Tap-and-drag is optional and can be enabled or disabled with
**libinput_device_config_tap_set_drag_enabled()**. Most devices have
tap-and-drag enabled by default.

.. note:: Dragging is always done with one finger. The number of fingers on
          the initial tap decide the type of button click. For example, to
          middle-click drag, tap with three fingers followed by a
          single-finger drag.

Also optional is a feature called "drag lock". With drag lock disabled, lifting
the finger will stop any drag process. When enabled, libinput will ignore a
finger up event during a drag process, provided the finger is set down again
within a implementation-specific timeout. Drag lock can be enabled and
disabled with **libinput_device_config_tap_set_drag_lock_enabled()**.
Note that drag lock only applies if tap-and-drag is be enabled.

.. figure:: tap-n-drag.svg
    :align: center

    Tap-and-drag process

The above diagram explains the process, a tap (a) followed by a finger held
down (b) starts the drag process and logically holds the left mouse button
down. A movement of the finger (c) will drag the selected item until the
finger is released (e). If needed and drag lock is enabled, the finger's
position can be reset by lifting and quickly setting it down again on the
touchpad (d). This will be interpreted as continuing move and is especially
useful on small touchpads or with slow pointer acceleration.
If drag lock is enabled, the release of the mouse buttons after the finger
release (e) is triggered by a timeout. To release the button immediately,
simply tap again (f).

If two fingers are supported by the hardware, a second finger can be used to
drag while the first is held in-place.

.. _hold_tap:

------------------------------------------------------------------------------
Hold-and-tap
------------------------------------------------------------------------------

libinput supports tapping with a finger one set of fingers while another set
of fingers is already in contact. This may speed up interactions where many
taps occur in the middle of pointer motion or scrolling, because it is no longer
necessary to lift the fingers responsible for motion and put them back down
after the tap and a :ref:`tapndrag` timeout (if enabled).

The tapping fingers can start tap-and-drag, as long as all holding fingers
are released alongside the tapping ones. In addition, the holding fingers are
allowed to be performing the latter half of tap-and-drag. In that case a tap
with the same number of fingers as the one which was used to start the drag
is ignored.

Hold-and-tap is optional and disabled by default, because it preempts some
protection against accidental short touches with a palm or thumb. It can be
enabled or disabled with **libinput_device_config_tap_set_hold_tap_enabled()**.
Example uses of hold-and-tap include:

- move the cursor to a clickable target with a one-finger motion, add a finger,
  release both (performs a click on the target)

- chaining the above: move to a target, tap with a second finger, move the first
  finger to the next target, tap with a second finger again, etc. (performs a
  click on each target)

- tap with one finger, quickly put one finger down to start a drag, move it,
  tap with two additional fingers (performs cursor movement with the left
  mouse button held down, followed by a click with another button while the left
  one is still down; in many environments this cancels a drag-and-drop and
  undoes its effects).

.. _tap_constraints:

------------------------------------------------------------------------------
Constraints while tapping
------------------------------------------------------------------------------

A couple of constraints apply to the contact to be converted into a press, the most common ones are:

- the touch down and touch up must happen within an implementation-defined timeout
- if a finger moves more than an implementation-defined distance while in contact, it's not a tap
- tapping within :ref:`clickpad software buttons <clickpad_softbuttons>` may not trigger an event
- a tap not meeting required pressure thresholds can be ignored as accidental touch
- a tap exceeding certain pressure thresholds can be ignored (see :ref:`palm_detection`)
- a tap on the edges of the touchpad can usually be ignored (see :ref:`palm_detection`)
