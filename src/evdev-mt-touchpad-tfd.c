#include "config.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "evdev-mt-touchpad.h"

/* when three fingers are detected, this is how long we wait to see if the user
actually intends a 3 finger gesture, or is transitioning to e.g. 4 fingers */
#define DEFAULT_DRAG3_WAIT_FOR_FINGERS_DURATION ms2us(50)
/* The interval between three fingers touching and a button press being 
performed, if the fingers remain stationary */
#define DEFAULT_DRAG3_INITIAL_DELAY ms2us(350)
/* The time window during which you can continue a 3 finger drag by reapplying 
three fingers. ~700-800 ms seems ideal. */
#define DEFAULT_DRAG3_WAIT_FOR_RESUME_DURATION ms2us(720)
/* The speed at which the *released* finger needs to travel for the drag to 
continue with a single finger */
#define DEFAULT_DRAG3_1F_CONTINUATION_SPEED 40 /* mm/s */

enum tfd_event {
	TFD_EVENT_MOTION,
	TFD_EVENT_TOUCH_COUNT_INCREASE,
	TFD_EVENT_TOUCH_COUNT_DECREASE,
	TFD_EVENT_BUTTON,
	TFD_EVENT_TAP,
	TFD_EVENT_TIMEOUT,
	TFD_EVENT_RESUME_TIMEOUT,
};

/*****************************************
 * TODO: provide a diagram
 * Look at the state diagram in doc/three-finger-drag-state-machine.svg
 * (generated with https://www.diagrams.net)
 *
 * Any changes in this file must be represented in the diagram.
 */

static inline const char*
tfd_state_to_str(enum tp_tfd_state state)
{
	switch(state) {
	CASE_RETURN_STRING(TFD_STATE_IDLE);
	CASE_RETURN_STRING(TFD_STATE_POSSIBLE_DRAG);
	CASE_RETURN_STRING(TFD_STATE_DRAG);
	CASE_RETURN_STRING(TFD_STATE_AWAIT_RESUME);
	CASE_RETURN_STRING(TFD_STATE_POSSIBLE_RESUME);
	}
	return NULL;
}

static inline const char*
tfd_event_to_str(enum tfd_event event)
{
	switch(event) {
	CASE_RETURN_STRING(TFD_EVENT_MOTION);
	// CASE_RETURN_STRING(TFD_EVENT_TOUCH_COUNT);
	CASE_RETURN_STRING(TFD_EVENT_TOUCH_COUNT_INCREASE);
	CASE_RETURN_STRING(TFD_EVENT_TOUCH_COUNT_DECREASE);
	CASE_RETURN_STRING(TFD_EVENT_BUTTON);
	CASE_RETURN_STRING(TFD_EVENT_TAP);
	CASE_RETURN_STRING(TFD_EVENT_TIMEOUT);
	CASE_RETURN_STRING(TFD_EVENT_RESUME_TIMEOUT);
	}
	return NULL;
}

static inline void
log_tfd_bug(struct tp_dispatch *tp, enum tfd_event event, int nfingers_down)
{
	evdev_log_bug_libinput(tp->device,
			       "invalid TFD event %s with %d fingers in state %s\n",
			       tfd_event_to_str(event),
				   nfingers_down,
			       tfd_state_to_str(tp->tfd.state));
}

static void
tp_tfd_notify(struct tp_dispatch *tp,
	      uint64_t time,
	      int nfingers,
	      enum libinput_button_state state)
{
	int32_t button;
	int32_t button_map[2][3] = {
		{ BTN_LEFT, BTN_RIGHT, BTN_MIDDLE },
		{ BTN_LEFT, BTN_MIDDLE, BTN_RIGHT },
	};

	assert(tp->tap.map < ARRAY_LENGTH(button_map));

	if (nfingers < 1 || nfingers > 3)
		return;

	button = button_map[tp->tap.map][nfingers - 1];

	if (state == LIBINPUT_BUTTON_STATE_PRESSED) {
		assert(!(tp->tfd.buttons_pressed & (1 << nfingers)));
		tp->tfd.buttons_pressed |= (1 << nfingers);
	}
	else {
		assert(tp->tfd.buttons_pressed & (1 << nfingers));
		tp->tfd.buttons_pressed &= ~(1 << nfingers);
	}

	evdev_pointer_notify_button(tp->device,
				    time,
				    button,
				    state);
}

static void
tp_tfd_set_button_press_delay_timer(struct tp_dispatch *tp, uint64_t time)//, 
										// uint64_t duration)
{
	// libinput_timer_set(&tp->tfd.timer, time + duration);
	libinput_timer_set(&tp->tfd.timer, time + DEFAULT_DRAG3_INITIAL_DELAY);
}

static void
tp_tfd_set_await_more_fingers_timer(struct tp_dispatch *tp, uint64_t time)//, 
										// uint64_t duration)
{
	// libinput_timer_set(&tp->tfd.timer, time + duration);
	libinput_timer_set(&tp->tfd.timer, time + DEFAULT_DRAG3_WAIT_FOR_FINGERS_DURATION);
}

static void
tp_tfd_set_await_resume_timer(struct tp_dispatch *tp, uint64_t time)
{
	libinput_timer_set(&tp->tfd.resume_timer, time + DEFAULT_DRAG3_WAIT_FOR_RESUME_DURATION);
}

static void
tp_tfd_clear_timer(struct tp_dispatch *tp)
{
	libinput_timer_cancel(&tp->tfd.timer);
}

static void
tp_tfd_clear_resume_timer(struct tp_dispatch *tp)
{
	libinput_timer_cancel(&tp->tfd.resume_timer);
}

static bool
tp_touch_active_for_tfd(const struct tp_dispatch *tp, const struct tp_touch *t)
{
	return (t->state == TOUCH_BEGIN || t->state == TOUCH_UPDATE) &&
		t->palm.state == PALM_NONE; //&&
		// !t->pinned.is_pinned &&
		// !tp_thumb_ignored_for_gesture(tp, t) &&

		// not sure what the reason is for these
		// tp_button_touch_active(tp, t) &&
		// tp_edge_scroll_touch_active(tp, t);
}


static struct device_coords
tp_get_aggregate_touches_coords(const struct tp_dispatch *tp, bool average)
{
	struct tp_touch *t;
	unsigned int i, nactive = 0;
	struct device_coords total = {0, 0};

	for (i = 0; i < tp->num_slots; i++) {
		t = &tp->touches[i];

		if (!tp_touch_active_for_tfd(tp, t))
			continue;

		nactive++;

		if (t->dirty) {
			total.x += t->point.x;
			total.y += t->point.y;
		}
	}

	if (!average || nactive == 0)
		return total;

	total.x /= nactive;
	total.y /= nactive;

	return total;
}

/* TODO: disable 3 finger gestures dynamically -- can't have both */

static void
tp_tfd_pin_fingers(struct tp_dispatch *tp)
{
	tp->tfd.cursor_pinned = true;
	tp->tfd.pinned_point = tp_get_aggregate_touches_coords(tp, true);
}

static void
tp_tfd_unpin_fingers(struct tp_dispatch *tp)
{
	tp->tfd.cursor_pinned = false;
}

static bool
tp_tfd_should_be_unpinned(const struct tp_dispatch *tp, struct tp_touch *t)
{
	struct phys_coords mm;
	struct device_coords delta;

	if (!tp->tfd.cursor_pinned)
		return true;

	// unsure why there was a call to abs() here
	// delta.x = abs(t->point.x - t->pinned.center.x);
	// delta.y = abs(t->point.y - t->pinned.center.y);

	/* TODO: this should correspond to whatever gesture.c does */
	/* TODO: can't gesture.c just use the touch with the largest delta so as to
	allow a single finger to control the cursor at the same speed as all fingers? */ 
	delta = tp_get_aggregate_touches_coords(tp, true);
	delta.x = delta.x - tp->tfd.pinned_point.x;
	delta.y = delta.y - tp->tfd.pinned_point.y;

	mm = evdev_device_unit_delta_to_mm(tp->device, &delta);

	/* 2.0 mm movement -> unpin */
	return (hypot(mm.x, mm.y) >= 2.0);
}

static void
tp_tfd_idle_handle_event(struct tp_dispatch *tp,
			 struct tp_touch *t,
			 enum tfd_event event, uint64_t time, int nfingers_down)
{
	switch (event) {
	case TFD_EVENT_TOUCH_COUNT_INCREASE:
	case TFD_EVENT_TOUCH_COUNT_DECREASE:
		if (nfingers_down == 3) {
			tp->tfd.state = TFD_STATE_POSSIBLE_DRAG;
			tp_tfd_set_button_press_delay_timer(tp, time);
		}
		break;
	case TFD_EVENT_MOTION:
		break;
	case TFD_EVENT_RESUME_TIMEOUT:
	case TFD_EVENT_TIMEOUT:
		log_tfd_bug(tp, event, nfingers_down);
		break; // bug
	case TFD_EVENT_TAP:
	case TFD_EVENT_BUTTON:
		break;
	}
}

/* We don't have the primary button pressed in this state; the 
press is delayed if the fingers have remained stationary */
static void
tp_tfd_possible_drag_handle_event(struct tp_dispatch *tp,
			      struct tp_touch *t,
			      enum tfd_event event, uint64_t time, int nfingers_down)
{	
	switch (event) {
	case TFD_EVENT_TOUCH_COUNT_INCREASE:
	case TFD_EVENT_TOUCH_COUNT_DECREASE:
		switch (nfingers_down) {
		case 3: 
			log_tfd_bug(tp, event, nfingers_down);
			break; // bug
		default:
			tp->tfd.state = TFD_STATE_IDLE;
			tp_tfd_clear_timer(tp);
			break;
		}
		break;
	case TFD_EVENT_MOTION:
		switch (nfingers_down) {
		default: 
			log_tfd_bug(tp, event, nfingers_down);
			break; // bug
		case 3:
			/* perform a press since it hasn't already been done by the timer */
			tp->tfd.state = TFD_STATE_DRAG;
			tp_tfd_notify(tp, time, 1, LIBINPUT_BUTTON_STATE_PRESSED);
			tp_tfd_clear_timer(tp);
		}
		break;
	case TFD_EVENT_RESUME_TIMEOUT:
		break;
	case TFD_EVENT_TIMEOUT:
		/* we've not moved our three fingers so we perform the press after the 
		initial delay */
		tp->tfd.state = TFD_STATE_DRAG;
		tp_tfd_notify(tp, time, 1, LIBINPUT_BUTTON_STATE_PRESSED);
		break;
	case TFD_EVENT_TAP:
	case TFD_EVENT_BUTTON:
		// TODO: undecided
		//tp->tfd.state = TFD_STATE_IDLE;
		//tp_tfd_clear_timer(tp);
		break;
	}
}

static void
tp_tfd_drag_handle_event(struct tp_dispatch *tp,
			      struct tp_touch *t,
			      enum tfd_event event, uint64_t time,
				   int nfingers_down)
{	
	switch (event) {
	// case TFD_EVENT_TOUCH_COUNT:
	case TFD_EVENT_TOUCH_COUNT_INCREASE:
	case TFD_EVENT_TOUCH_COUNT_DECREASE:
		switch (nfingers_down) {
		case 0: 
		case 1:
			tp_tfd_pin_fingers(tp);
			/* removing all, or all but one, fingers gives you ~0.7 seconds to 
			place three fingers back on the touchpad before the drag ends */
			tp_tfd_set_await_resume_timer(tp, time);
			tp->tfd.state = TFD_STATE_AWAIT_RESUME;

			// tp_tfd_pin_fingers(tp);
			// tp_tfd_set_await_resume_timer(tp, time);
			// tp_tfd_set_await_more_fingers_timer(tp, time);
			// tp->tfd.state = TFD_STATE_POSSIBLE_RESUME;

			break;
		default: 
			break;
		}
		break;
	case TFD_EVENT_MOTION:
		/* TODO: Future improvement: When one finger moves considerably 
		faster than the others, don't average their deltas for cursor 
		position updates -- use the fastest finger only */
		break;
	case TFD_EVENT_RESUME_TIMEOUT:
	case TFD_EVENT_TIMEOUT:
		log_tfd_bug(tp, event, nfingers_down);
		break; // bug
	case TFD_EVENT_TAP:
		break;
	case TFD_EVENT_BUTTON:
		tp_tfd_unpin_fingers(tp);
		tp->tfd.state = TFD_STATE_IDLE;
		tp_tfd_clear_resume_timer(tp);
		tp_tfd_notify(tp, time, 1, LIBINPUT_BUTTON_STATE_RELEASED);
		break;
	}
}

/* Drag-lock; After leaving 3 finger dragging there's a small time window where you can 
resume the drag with 3 fingers. */
static void
tp_tfd_await_resume_handle_event(struct tp_dispatch *tp,
				  struct tp_touch *t,
				  enum tfd_event event, uint64_t time,
				   int nfingers_down)
{
	switch (event) {
		/* we enter this state with 1 or 0 fingers */
	case TFD_EVENT_TOUCH_COUNT_DECREASE:
		/* decreasing the amount of fingers does not concern us in this state 
		as long as an increase to > 3 invariably moves to another state */

		/* TODO: Bug: very quick drags will immediately terminate because of this
		in combination with breaking out of drag during 1 finger or 2 fingers MOTION.
		
		Solution (?): POSSIBLE_BREAK_OUT state concerned with touch count decrease */
		break;
	case TFD_EVENT_TOUCH_COUNT_INCREASE:
		switch (nfingers_down) {
			case 0:
				log_tfd_bug(tp, event, nfingers_down);
				break; // bug
			case 1:
			case 2:
				// tp_tfd_pin_fingers(tp);
				// tp_tfd_set_await_more_fingers_timer(tp, time);
				// tp->tfd.state = TFD_STATE_POSSIBLE_RESUME;
				// break;
			case 3:
				/* Exactly three fingers are required to resume dragging, in order to 
				enable instant scroll and cursor control. */
				// tp->tfd.state = TFD_STATE_POSSIBLE_RESUME;
				// tp_tfd_set_timer_wait_for_fingers(tp);

				// tp_tfd_unpin_fingers(tp);
				// tp_tfd_clear_resume_timer(tp);

				tp_tfd_pin_fingers(tp);
				tp_tfd_set_await_more_fingers_timer(tp, time);

				// /* when 3 fingers are confirmed, immediately reset drag-lock timeout so
				// that 3f drags that are shorter than more_fingers_timer will actually 
				// prevents drag-lock from timing out */
				// tp_tfd_set_await_resume_timer(tp, time);
				// tp_tfd_clear_resume_timer(tp);

				/* time to disambiguate from a 4 finger gesture */
				tp->tfd.state = TFD_STATE_POSSIBLE_RESUME;

				break;
			default:
				// TODO: undecided, but this behavior shouldn't be an issue
				tp_tfd_unpin_fingers(tp);
				tp->tfd.state = TFD_STATE_IDLE;
				tp_tfd_clear_resume_timer(tp);
				tp_tfd_notify(tp, time, 1, LIBINPUT_BUTTON_STATE_RELEASED);
				break;
		}
		break;
	case TFD_EVENT_MOTION:
		/* Zero, one, or two fingers can be touching. */
		switch (nfingers_down) {
			case 3:
				log_tfd_bug(tp, event, nfingers_down);
				break; // bug
			default: // bug, currently
				log_tfd_bug(tp, event, nfingers_down);
				break;
				// // TODO: undecided
				// tp_tfd_unpin_fingers(tp);
				// tp->tfd.state = TFD_STATE_IDLE;
				// tp_tfd_clear_resume_timer(tp);
				// tp_tfd_notify(tp, time, 1, LIBINPUT_BUTTON_STATE_RELEASED);
				// break;
			case 1:
			case 2:
				if (tp_tfd_should_be_unpinned(tp, t)) {
					tp_tfd_unpin_fingers(tp);
					tp->tfd.state = TFD_STATE_IDLE;
					tp_tfd_clear_resume_timer(tp);
					tp_tfd_notify(tp, time, 1, LIBINPUT_BUTTON_STATE_RELEASED);
				}
				break;
		}
		break;
	case TFD_EVENT_RESUME_TIMEOUT: 
		/* the drag was not resumed */
		tp_tfd_unpin_fingers(tp);
		tp->tfd.state = TFD_STATE_IDLE;
		tp_tfd_notify(tp, time, 1, LIBINPUT_BUTTON_STATE_RELEASED);
		break;
	case TFD_EVENT_TIMEOUT:
		log_tfd_bug(tp, event, nfingers_down);
		break; // bug
	case TFD_EVENT_TAP:
	case TFD_EVENT_BUTTON:
		tp_tfd_unpin_fingers(tp);
		tp->tfd.state = TFD_STATE_IDLE;
		tp_tfd_clear_resume_timer(tp);
		tp_tfd_notify(tp, time, 1, LIBINPUT_BUTTON_STATE_RELEASED);
		break;
	}
}

/* Waiting for more fingers. Fingers have been detected, but it might be 
a transitory phase towards 2, 4 or more fingers, which should not resume the
drag. */
static void
tp_tfd_possible_resume_handle_event(struct tp_dispatch *tp,
				  struct tp_touch *t,
				  enum tfd_event event, uint64_t time,
				   int nfingers_down)
{
	switch (event) {
	case TFD_EVENT_TOUCH_COUNT_INCREASE:
		// bool did_transition_from_three_fingers = tp->tfd.finger_count == 3;
		switch (nfingers_down) {
		case 0: 
		case 1:
		case 2:
			// assert(false);
			// tp_tfd_pin_fingers(tp);
			// if (did_transition_from_three_fingers)
			// 	tp_tfd_set_await_resume_timer(tp, time);
			break;
		case 3:
			// assert(false);
			// tp_tfd_pin_fingers(tp);
			// /* when 3 fingers are confirmed, immediately reset drag-lock timeout so
			// that 3f drags that are shorter than more_fingers_timer will actually 
			// prevent drag-lock from timing out */
			// // TODO: this is wrong. should be set when transitioning from 3
			// tp_tfd_set_await_resume_timer(tp, time);
			// tp_tfd_clear_resume_timer(tp);
			break;
		default:
			tp_tfd_unpin_fingers(tp);
			tp->tfd.state = TFD_STATE_IDLE;
			tp_tfd_clear_resume_timer(tp);
			tp_tfd_clear_timer(tp);
			tp_tfd_notify(tp, time, 1, LIBINPUT_BUTTON_STATE_RELEASED);
			break;
		}
		break;
	case TFD_EVENT_MOTION:
		switch (nfingers_down) {
		case 0:
		case 1:
		case 2:
			// assert(false);
			break;
		case 3:
			tp_tfd_unpin_fingers(tp);
			tp_tfd_clear_resume_timer(tp);
			tp_tfd_clear_timer(tp);
			tp->tfd.state = TFD_STATE_DRAG;
			break;
		default:
			/* should have left the state already */

			// TODO: undecided
			// tp_tfd_unpin_fingers(tp);
			// tp->tfd.state = TFD_STATE_IDLE;
			// tp_tfd_clear_resume_timer(tp);
			// tp_tfd_clear_timer(tp);
			// tp_tfd_notify(tp, time, 1, LIBINPUT_BUTTON_STATE_RELEASED);
			break;
		}
		break;
	case TFD_EVENT_RESUME_TIMEOUT: 
		/* immediately evaluate whether to resume. TODO: unwise? */
		switch (nfingers_down) {
		case 3:
			tp_tfd_unpin_fingers(tp);
			tp_tfd_clear_timer(tp);
			tp->tfd.state = TFD_STATE_DRAG;
			break;
		default:
			tp_tfd_unpin_fingers(tp);
			tp_tfd_clear_timer(tp);
			tp->tfd.state = TFD_STATE_IDLE;
			tp_tfd_notify(tp, time, 1, LIBINPUT_BUTTON_STATE_RELEASED);
			break;
		}
		break;
	case TFD_EVENT_TOUCH_COUNT_DECREASE:
		/* a decrease forces immediate evaluation as if the timer had fired */
		/* TODO: might be beneficial with some debouncing on touch count decrease
		as well, in order to not react too quickly... */
		tp_tfd_clear_timer(tp);
		/* fallthrough */
	case TFD_EVENT_TIMEOUT:
		/* time to check whether we have 3 fingers touching */
		switch (nfingers_down) {
		case 0:
		case 1:
		case 2:
			// tp_tfd_pin_fingers(tp);
			// tp_tfd_set_await_resume_timer(tp, time);
			tp->tfd.state = TFD_STATE_AWAIT_RESUME;
			break;
		case 3:
			tp_tfd_unpin_fingers(tp);
			tp_tfd_clear_resume_timer(tp);
			tp->tfd.state = TFD_STATE_DRAG;
			break;
		default:
			tp_tfd_unpin_fingers(tp);
			tp->tfd.state = TFD_STATE_IDLE;
			tp_tfd_clear_resume_timer(tp);
			tp_tfd_notify(tp, time, 1, LIBINPUT_BUTTON_STATE_RELEASED);
			break;
		}
		break;
	case TFD_EVENT_TAP:
	case TFD_EVENT_BUTTON:
		tp_tfd_unpin_fingers(tp);
		tp->tfd.state = TFD_STATE_IDLE;
		tp_tfd_clear_resume_timer(tp);
		tp_tfd_clear_timer(tp);
		tp_tfd_notify(tp, time, 1, LIBINPUT_BUTTON_STATE_RELEASED);
		break;
	}
}

// 			TODO: It's too easy to trigger 3fd while scrolling and a third finger 
// 			touches momentarily. */			

static void
tp_tfd_handle_event(struct tp_dispatch *tp,
		    struct tp_touch *t,
		    enum tfd_event event,
		    uint64_t time,
			int nfingers_down)
{
	enum tp_tfd_state previous_state;
	// int nfingers_down = tp->tfd.finger_count;
	previous_state = tp->tfd.state;

	assert(nfingers_down >= 0);
	// assert(nfingers_down < 6); // TODO: temp, remove

	switch (event) {
		case TFD_EVENT_MOTION:
		case TFD_EVENT_TOUCH_COUNT_INCREASE:
			assert(nfingers_down > 0);
			break;
		case TFD_EVENT_TOUCH_COUNT_DECREASE:
			// assert(nfingers_down < 5);
			break;
		default: 
			break;
	}

	switch(tp->tfd.state) {
	case TFD_STATE_IDLE:
		tp_tfd_idle_handle_event(tp, t, event, time, nfingers_down);
		break;
		
	case TFD_STATE_POSSIBLE_DRAG:
		tp_tfd_possible_drag_handle_event(tp, t, event, time, nfingers_down);
		break;

	case TFD_STATE_DRAG:
		tp_tfd_drag_handle_event(tp, t, event, time, nfingers_down);
		break;

	case TFD_STATE_AWAIT_RESUME:
		tp_tfd_await_resume_handle_event(tp, t, event, time, nfingers_down);
		break;

	case TFD_STATE_POSSIBLE_RESUME:
		tp_tfd_possible_resume_handle_event(tp, t, event, time, nfingers_down);
		break;
	}

	if (previous_state != tp->tfd.state)
		evdev_log_debug(tp->device,
			  "tfd: touch %d (%s), tfd state %s → %s → %s\n",
			  t ? (int)t->index : -1,
			  t ? touch_state_to_str(t->state) : "",
			  tfd_state_to_str(previous_state),
			  tfd_event_to_str(event),
			  tfd_state_to_str(tp->tfd.state));
}

/* TODO: too arbitrary... need to decide on the semantics of the MOTION event properly */
#define DEFAULT_TFD_MOVE_THRESHOLD 0.1 //1.0 //1.3 /* mm */

// TODO: how often do clients get motion updates? Isn't that the granularity we 
// want as well? Otherwise we might miss cursor motion, and the button press 
// will then occur one or more pixels off from the intended position.

/* TODO: receive input from gesture.c? */

/* reused tap logic */
/* TODO: do I really need a threshold? The motion could be handled in event handlers
directly instead */
static bool
tp_tfd_exceeds_motion_threshold(struct tp_dispatch *tp,
				struct tp_touch *t)
{
	struct phys_coords mm =
		tp_phys_delta(tp, device_delta(t->point, t->tfd.previous));

	/* if we have more fingers down than slots, we know that synaptics
	 * touchpads are likely to give us pointer jumps.
	 * This triggers the movement threshold, making three-finger taps
	 * less reliable (#101435)
	 *
	 * This uses the real nfingers_down, not the one for taps.
	 */
	if (tp->device->model_flags & EVDEV_MODEL_SYNAPTICS_SERIAL_TOUCHPAD &&
	    (tp->nfingers_down > 2 || tp->old_nfingers_down > 2) &&
	    (tp->nfingers_down > tp->num_slots ||
	     tp->old_nfingers_down > tp->num_slots)) {
		return false;
	}

	/* Semi-mt devices will give us large movements on finger release,
	 * depending which touch is released. Make sure we ignore any
	 * movement in the same frame as a finger change.
	 */
	if (tp->semi_mt && tp->nfingers_down != tp->old_nfingers_down)
		return false;

	double threshold = DEFAULT_TFD_MOVE_THRESHOLD;
	if (tp->tfd.state == TFD_STATE_POSSIBLE_DRAG) {
		// TODO: have to figure out something better
		// MOTION events are too decoupled from what's required to actually move
		// the cursor. TODO: look it up

		/* the default threshold is too fine-grained for detection of initial 
		button press, but is kind of needed for the unpin distance to be 
		somewhat accurate in the other states a.t.m. */
		threshold = 1.3; // same as tap.c
	}

	return length_in_mm(mm) > threshold;
}

// unused
// static bool
// tp_tfd_enabled(struct tp_dispatch *tp)
// {
// 	return tp->tfd.enabled && !tp->tfd.suspended;
// }

// unused
// static bool
// tp_touch_active_for_tfd_including_edge_palm(const struct tp_dispatch *tp, const struct tp_touch *t)
// {
// 	return (t->state == TOUCH_BEGIN || t->state == TOUCH_UPDATE) &&
// 		(t->palm.state == PALM_NONE || t->palm.state == PALM_EDGE); //&&
// 		// !t->pinned.is_pinned &&
// 		// !tp_thumb_ignored_for_gesture(tp, t) &&

// 		// not sure what the reason is for these
// 		// tp_button_touch_active(tp, t) &&
// 		// tp_edge_scroll_touch_active(tp, t);
// }

void
tp_tfd_handle_state(struct tp_dispatch *tp, uint64_t time)
{
	unsigned int active_touches = 0;
	unsigned int active_touches_excluding_edge_palm = 0;
	// unsigned int active_touches_including_edge_palm = 0;
	struct tp_touch *t;

	tp_for_each_touch(tp, t) {
		if (tp_touch_active_for_tfd(tp, t))
			active_touches_excluding_edge_palm++;		
		// if (tp_touch_active_for_tfd_including_edge_palm(tp, t))
		// 	active_touches_including_edge_palm++;
	}

	// if (active_touches_excluding_edge_palm >= 2)
	// 	active_touches = active_touches_including_edge_palm;
	// else
		active_touches = active_touches_excluding_edge_palm;


	if (active_touches < tp->tfd.finger_count) {
		tp_tfd_handle_event(tp, t, TFD_EVENT_TOUCH_COUNT_DECREASE, time, active_touches);
	}
	else if (active_touches > tp->tfd.finger_count) {
		tp_tfd_handle_event(tp, t, TFD_EVENT_TOUCH_COUNT_INCREASE, time, active_touches);
	}

	// TODO: consider whether transparent debouncing has any value for TFD.
	// Maybe not; transitions to cursor control and scrolling must never
	// affect the position for button release, which seems unavoidable with
	// transparent debouncing -- unless the pin/unpin decisions are also broken 
	// out from event handling...

	// if (!tp_tfd_enabled(tp))
	// 	return 0;

	/* Handle queued button pressed events from clickpads. */
	if (/* tp->buttons.is_clickpad && */ tp->queued & TOUCHPAD_EVENT_BUTTON_PRESS)
		tp_tfd_handle_event(tp, NULL, TFD_EVENT_BUTTON, time, active_touches);

	bool motion_occurred = false;

	tp_for_each_touch(tp, t) {
		// TODO: don't know the semantics of `dirty`...
		if (!t->dirty || t->state == TOUCH_NONE)
			continue;

		if(!tp_touch_active_for_tfd(tp, t))
			continue;

		if (t->state == TOUCH_HOVERING)
			continue;

		if (t->state == TOUCH_BEGIN) {
			t->tfd.previous = t->point;
		}
		else if (t->state == TOUCH_UPDATE) {
			if (tp_tfd_exceeds_motion_threshold(tp, t)) { // t->tfd.previous.x != t->point.x || t->tfd.previous.y != t->point.y) {
				motion_occurred = true; 
			}
		}
	}

	if (motion_occurred) {
		tp_tfd_handle_event(tp, t, TFD_EVENT_MOTION, time, active_touches);
	}

	/* finally, update additional state */
	if (motion_occurred)
		t->tfd.previous = t->point;

	tp->tfd.finger_count = active_touches;

	// assert(tp->tfd.nfingers_down <= tp->nfingers_down);
	// if (tp->nfingers_down == 0)
	// 	assert(tp->tfd.nfingers_down == 0);
}

static void
tp_tfd_handle_timeout(uint64_t time, void *data)
{
	struct tp_dispatch *tp = data;

	tp_tfd_handle_event(tp, NULL, TFD_EVENT_TIMEOUT, time, tp->tfd.finger_count);
}

static void
tp_tfd_handle_resume_timeout(uint64_t time, void *data)
{
	struct tp_dispatch *tp = data;

	tp_tfd_handle_event(tp, NULL, TFD_EVENT_RESUME_TIMEOUT, time, tp->tfd.finger_count);
}

/* when a tap occurs the drag can be finished ahead of time if in the waiting state */
void
tp_tfd_handle_tap(struct tp_dispatch *tp, uint64_t time)
{
	switch (tp->tfd.state) {
		case TFD_STATE_AWAIT_RESUME:
		case TFD_STATE_POSSIBLE_RESUME:
			tp_tfd_handle_event(tp, NULL, TFD_EVENT_TAP, time, tp->tfd.finger_count);
			break;
		case TFD_STATE_IDLE:
		case TFD_STATE_POSSIBLE_DRAG:
		case TFD_STATE_DRAG:
			break;
	}
}


	
void
tp_init_tfd(struct tp_dispatch *tp)
{
	char timer_name[64];

	// tp->tfd.config.count = tp_tfd_config_count;
	// tp->tfd.config.set_enabled = tp_tfd_config_set_enabled;
	// tp->tfd.config.get_enabled = tp_tfd_config_is_enabled;
	// tp->tfd.config.get_default = tp_tfd_config_get_default;
	// tp->tfd.config.set_map = tp_tfd_config_set_map;
	// tp->tfd.config.get_map = tp_tfd_config_get_map;
	// tp->tfd.config.get_default_map = tp_tfd_config_get_default_map;
	// tp->tfd.config.set_drag_enabled = tp_tfd_config_set_drag_enabled;
	// tp->tfd.config.get_drag_enabled = tp_tfd_config_get_drag_enabled;
	// tp->tfd.config.get_default_drag_enabled = tp_tfd_config_get_default_drag_enabled;
	// tp->tfd.config.set_draglock_enabled = tp_tfd_config_set_draglock_enabled;
	// tp->tfd.config.get_draglock_enabled = tp_tfd_config_get_draglock_enabled;
	// tp->tfd.config.get_default_draglock_enabled = tp_tfd_config_get_default_draglock_enabled;
	// tp->device->base.config.tap = &tp->tfd.config;

	tp->tfd.state = TFD_STATE_IDLE;
	tp->tfd.enabled = true; //tp_tfd_default(tp->device);
	tp->tfd.suspended = false;
	//tp->tfd.map = LIBINPUT_CONFIG_TAP_MAP_LRM;
	//tp->tfd.want_map = tp->tfd.map;
	//tp->tfd.drag_enabled = tp_drag_default(tp->device);
	//tp->tfd.drag_lock_enabled = tp_drag_lock_default(tp->device);
	tp->tfd.three_finger_dragging_enabled = 1;
	tp->tfd.finger_count = 0;

	snprintf(timer_name,
		 sizeof(timer_name),
		 "%s tfd",
		 evdev_device_get_sysname(tp->device));
	libinput_timer_init(&tp->tfd.timer,
			    tp_libinput_context(tp),
			    timer_name,
			    tp_tfd_handle_timeout, tp);

	snprintf(timer_name,
		 sizeof(timer_name),
		 "%s tfd resume",
		 evdev_device_get_sysname(tp->device));
	libinput_timer_init(&tp->tfd.resume_timer,
			    tp_libinput_context(tp),
			    timer_name,
			    tp_tfd_handle_resume_timeout, tp);
}

bool
tp_tfd_dragging(const struct tp_dispatch *tp)
{
	switch (tp->tfd.state) {
		case TFD_STATE_DRAG:
		case TFD_STATE_AWAIT_RESUME:
		case TFD_STATE_POSSIBLE_RESUME:
			return true;
		default:
			return false;
	}
}

