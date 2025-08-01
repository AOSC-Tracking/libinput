-- SPDX-License-Identifier: MIT
--
-- This plugin implements a very simple version of disable-while-typing.
-- It monitors all keyboard devices and if any of them send an event,
-- any touchpad device is disabled for 2 seconds.
-- And "disabled" means any event from that touchpad is simply
-- discarded.
--
-- Install this file in /etc/libinput/plugins and
--
-- UNCOMMENT THIS LINE TO ACTIVATE THE PLUGIN
-- libinput:register({1})

tp_enabled = true

libinput:connect("timer-expired", function(_, now)
    log.debug("touchpad enabled")
    tp_enabled = true
end)

libinput:connect("new-evdev-device", function (_, device)
    local props = device:udev_properties()
    if props.ID_INPUT_KEYBOARD then
        device:connect("evdev-frame", function (_, _, _)
            libinput:timer_set_relative(2000000)
            if tp_enabled then
                log.debug("touchpad disabled")
                tp_enabled = false
            end
        end)
    elseif props.ID_INPUT_TOUCHPAD then
        log.debug("Touchpad detected: " .. device:name())
        device:connect("evdev-frame", function (_, frame, _)
            if not tp_enabled then
                -- Returning an empty table discards the event.
                return {}
            end
        end)
    end
end)
