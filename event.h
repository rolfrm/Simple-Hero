// requires bitguy.h
typedef enum{
  KEY,
  MOUSE_MOTION,
  MOUSE_BUTTON,
  MOUSE_WHEEL,
  JOY_AXIS,
  JOY_BALL,
  JOY_HAT,
  JOY_BUTTON,
  JOY_DEVICE,
  QUIT,
  EVENT_UNKNOWN
}event_type;


typedef struct{
  double x,y;
  double relx,rely;
}mouse_motion_event;

typedef enum{
  LEFT,
  RIGHT,
  MIDDLE  
}mouse_button;

typedef enum{
  BUTTON_DOWN,
  BUTTON_UP
}button_state;

typedef struct{
  mouse_button button;
  button_state state;
}mouse_button_event;

typedef enum{
  KEY_BACKSPACE,
  KEY_TAB,
  KEY_CLEAR,
  KEY_RETURN,
  KEY_PAUSE,
  KEY_ESCAPE,
  KEY_SPACE,
  KEY_EXCLAIM,
  KEY_QUOTEDBL,
  KEY_HASH,
  KEY_DOLLAR,
  KEY_AMPERSAND,
  KEY_QUOTE,
  KEY_LEFTPAREN,
  KEY_RIGHTPAREN,
  KEY_ASTERISK,
  KEY_PLUS,
  KEY_COMMA,
  KEY_MINUS,
  KEY_PERIOD,
  KEY_SLASH,
  KEY_0,
  KEY_1,
  KEY_2,
  KEY_3,
  KEY_4,
  KEY_5,
  KEY_6,
  KEY_7,
  KEY_8,
  KEY_9,
  KEY_COLON,
  KEY_SEMICOLON,
  KEY_LESS,
  KEY_EQUALS,
    KEY_GREATER,
  KEY_QUESTION,
  KEY_AT,
  KEY_LEFTBRACKET,
  KEY_BACKSLASH,
  KEY_RIGHTBRACKET,
  KEY_CARET,
  KEY_UNDERSCORE,
  KEY_BACKQUOTE,
  KEY_a,
  KEY_b,
  KEY_c,
  KEY_d,
  KEY_e,
  KEY_f,
  KEY_g,
  KEY_h,
  KEY_i,
  KEY_j,
  KEY_k,
  KEY_l,
  KEY_m,
  KEY_n,
  KEY_o,
  KEY_p,
  KEY_q,
  KEY_r,
  KEY_s,
  KEY_t,
  KEY_u,
  KEY_v,
  KEY_w,
  KEY_x,
  KEY_y,
  KEY_z,
  KEY_DELETE,
  KEY_KP,
  KEY_KP1,
  KEY_KP2,
  KEY_KP3,
  KEY_KP4,
  KEY_KP5,
  KEY_KP6,
  KEY_KP7,
  KEY_KP8,
  KEY_KP9,
  KEY_KP_PERIOD,
  KEY_KP_DIVIDE,
  KEY_KP_MULTIPLY,
  KEY_KP_MINUS,
  KEY_KP_PLUS,
  KEY_KP_ENTER,
  KEY_KP_EQUALS,
  KEY_UP,
  KEY_DOWN,
  KEY_RIGHT,
  KEY_LEFT,
  KEY_INSERT,
  KEY_HOME,
  KEY_END,
  KEY_PAGEUP,
  KEY_PAGEDOWN,
  KEY_F1,
  KEY_F2,
  KEY_F3,
  KEY_F4,
  KEY_F5,
  KEY_F6,
  KEY_F7,
  KEY_F8,
  KEY_F9,
  KEY_F10,
  KEY_F11,
  KEY_F12,
  KEY_F13,
  KEY_F14,
  KEY_F15,
  KEY_NUMLOCK,
  KEY_CAPSLOCK,
  KEY_SCROLLOCK,
  KEY_RSHIFT,
  KEY_LSHIFT,
  KEY_RCTRL,
  KEY_LCTRL,
  KEY_RALT,
  KEY_LALT,
  KEY_RMETA,
  KEY_LMETA,
  KEY_LSUPER,
  KEY_RSUPER,
  KEY_MODE,
  KEY_HELP,
  KEY_PRINT,
  KEY_SYSREQ,
  KEY_BREAK,
  KEY_MENU,
  KEY_POWER,
  KEY_EURO
}keysym;

typedef enum{
  KEYDOWN,
  KEYUP
}key_event_type;

typedef struct{
  key_event_type type;
  keysym sym;
}key_event;

typedef struct{
  keysym sym;
  char charcode;
  const char * description;
}keysym_descr;

keysym_descr keysym_descr_from_keysym(keysym sym);

typedef struct{
  int joystick_id;
  u8 axis;
  double value;

}joy_axis_event;

typedef struct{
  int joystick_id;
  u8 button;
  button_state state;
}joy_button_event;

typedef struct{
  event_type type;
  u32 id;
  u32 timestamp;
  union{
    key_event key;
    mouse_motion_event mouse_motion;
    mouse_button_event mouse_button;
    joy_axis_event joy_axis;
    joy_button_event joy_button;
  };

}event;
