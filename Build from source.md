# Build from source

## macOS

```sh
brew install qt@5
```

```sh
export PATH=/usr/local/opt/qt@5/bin:$PATH
export LDFLAGS=-L/usr/local/opt/qt@5/lib
export CPPFLAGS=-I/usr/local/opt/qt@5/include
export PKG_CONFIG_PATH=/usr/local/opt/qt@5/lib/pkgconfig
```

```sh
qmake -makefile application/application.pro
```

You can ignore the following output:

```output
Info: creating stash file .qmake.stash
Project WARNING: Qt has only been tested with version 11.0 of the platform SDK, you're using .
Project WARNING: This is an unsupported configuration. You may experience build issues, and by using
Project WARNING: the 12.3 SDK you are opting in to new features that Qt has not been prepared for.
Project WARNING: Please downgrade the SDK you use to build your app to version 11.0, or configure
Project WARNING: with CONFIG+=sdk_no_version_check when running qmake to silence this warning.
```

Then:

```sh
make
```

You are done. You can run the app via:

```sh
open dust3d.app
```