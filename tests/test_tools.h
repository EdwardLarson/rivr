#define Assert(_condition_, _msg_, _val_) if (!(_condition_)){ printf("Assertion failed: %s\n\t<%d>\n", _msg_, _val_); return 0; }
