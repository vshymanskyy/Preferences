# Testing

## Quick test

```sh
rm -rf .pio
pio pkg pack ../ --output Preferences.tar.gz

# Run tests
pio test -e native

# Check builds
pio test -vv --without-uploading --without-testing
```

## Rebuild with logs enabled

Edit `Preferences.cpp`, uncomment `#define NVS_LOG`
