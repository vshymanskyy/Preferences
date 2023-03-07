# Testing

## Quick test

```sh
pio test -e esp8266
```

## Full rebuild with logs enabled

Edit `Preferences.cpp`, uncomment `#define NVS_LOG`

Then run:

```sh
rm -rf .pio && pio test -e esp8266 -v
```
