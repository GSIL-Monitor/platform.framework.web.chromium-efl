## Introduction

chromium-efl is a Chromium/Blink engine port to tizen/efl platform. The port
implements Chromium/Blink platform APIs.

It also exposes a webview API implementation based on chromium-efl port. It is
supposed to be completely source and binary compatible with EFL-WebKit2.

## Details

1. gclient pulls chromium-efl into "src/tizen_src".
2. The it runs 2 hooks in order to get the rest of the source:

```
* generate-gclient-efl: .gclient-efl is created by running
  src/tizen_src/scripts/generate_gclient-efl.py (this is a fork
  of the same script in crosswalk repository).
* fetch-deps: It actually fetches all depedencies based on .gclient-efl.
```

## Procedure

1. Auto-generate gclient's configuration file (.gclient):

```
gclient config --name=src/tizen_src ssh://165.213.202.130:29418/webplatform/chromium-efl@beta/m42_2214_t
```

2. gclient sync

## Building

* For Desktop build

    $ ./build/build_desktop.sh [-h|--help] [--skip-gyp] [--skip-ninja] [--ccache] [--debug]

* For Mobile build

    $ build/build_mobile.sh [--clean] [--debug] [--skip-gyp] [--skip-ninja] [--define 'nodebug 1']
                            [--rpmlint] [--ccache] [--gbs-debug]

      [--define '_debug_mode 1'] or [--debug]       perform debug build (default : release)
      [--define '_skip_gyp 1'] or [--skip-gyp]      skip gyp generation (default : gyp generate)
      [--define '_skip_ninja 1'] or [--skip-ninja]  skip ninja execution (default : ninja executes)
      [--define 'nodebug 1']                        omit creation of debug packages
                                                    (default: build debug packages too)
                                                    Note: To let binaries to be recreated without debug symbols,
                                                    this should be preceded by removing build directory.
      [--rpmlint]                                   Enabling rpmlint on tizen v3.0
                                                    Note: By default, it is disabled.
      [--ccache]                                    see ### Using ccache inside gbs
      [--gbs-debug]                                 Run gbs in debug mode

* For TV build

    $ build/build_tv.sh [--clean] [--debug] [--skip-gyp] [--skip-ninja] [--define 'nodebug 1']
                        [--rpmlint] [--ccache] [--gbs-debug]

      [--define '_debug_mode 1'] or [--debug]       perform debug build (default : release)
      [--define '_skip_gyp 1'] or [--skip-gyp]      skip gyp generation  (default : gyp generate)
      [--define '_skip_ninja 1'] or [--skip-ninja]  skip ninja execution (default : ninja executes)
      [--define 'nodebug 1']                        omit creation of debug packages
                                                    (default: build debug packages too)
                                                    Note: To let binaries to be recreated without debug symbols,
                                                    this should be preceded by removing build directory.
      [--rpmlint]                                   Enabling rpmlint on tizen v3.0
                                                    Note: By default, it is disabled.
      [--ccache]                                    see ### Using ccache inside gbs
      [--gbs-debug]                                 Run gbs in debug mode


* For Emulator build

    $ build/build_emulator.sh mobile/tv [--clean] [--debug] [--skip-gyp] [--skip-ninja]
                                        [--define 'nodebug 1'] [--ccache] [--gbs-debug]

      [--define '_debug_mode 1'] or [--debug]       perform debug build (default : release)
      [--define '_skip_gyp 1'] or [--skip-gyp]      skip gyp generation  (default : gyp generate)
      [--define '_skip_ninja 1'] or [--skip-ninja]  skip ninja execution (default : ninja executes)
      [--define 'nodebug 1']                        omit creation of debug packages
                                                    (default: build debug packages too)
                                                    Note: To let binaries to be recreated without debug symbols,
                                                    this should be preceded by removing build directory.
      [--ccache]                                    see ### Using ccache inside gbs
      [--gbs-debug]                                 Run gbs in debug mode

## Using ccache inside gbs

To use ccache for faster full builds use --ccache parameter:
gbs build --ccache ...

Prerequisites:
* 10GB free space

ccache directory will be created with profile and architecture prefix e.g:
out.mobile.arm.ccache

## Coding style

Internally we use the chromium coding style: http://www.chromium.org/developers/coding-style.
For public headers we follow efl style.

## License

Chromium-efl's code uses the BSD license, see our `LICENSE` file.
