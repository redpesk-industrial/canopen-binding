# Installation

## From package

You can run the same command on a target runing a redpesk OS or in the [SDK container]({% chapter_link sdk-container-doc.overview %}) (development mode).

```bash
dnf install canopen-binding
```

## From sources

When developing inside the SDK container, to install the build dependencies, run the following command:

```bash
dnf builddep canopen-binding
```

Then clone and build from sources.

```bash
git clone https://github.com/redpesk-industrial/canopen-binding.git
cd canopen-binding
mkdir build
cd build
export PKG_CONFIG_PATH=$HOME/.local/lib64/pkgconfig:$HOME/.local/lib/pkgconfig:$PKG_CONFIG_PATH
cmake .. -DCMAKE_INSTALL_PREFIX=$HOME/.local
make
```

> Note: To rebuild all (including application framework) from sources, please refer to this [chapter]({% chapter_link host-build-doc.build-framework-on-your-computer %}).
