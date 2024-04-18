# Installation

## Easy install

This binding is available in the redpesk repo, so if you are running
on one of the supported Linux distros you can install it with your
package manager :

* Declare redpesk repository:
  [(see doc)]({% chapter_link host-configuration-doc.setup-your-build-host %})

* Install the binding :

    ``` bash
    # Fedora / redpesk :
    sudo dnf install canopen-binding

    # Ubuntu / Debian :
    sudo apt install canopen-binding

    # OpenSUSE :
    sudo zypper install canopen-binding
    ```

## Install from sources

### Dependencies

* redpesk binding development files 'afb-binding-dev' or 'afb-binding-devel'
* redpesk helpers 'afb-helpers4-static'
* lely-core CANopen library
* for running, redpesk application framework 'afb-binder'

> Note: _use *-dev or *-devel packages if available_

#### redpesk dependencies

Add redpesk repositories and install the Application Framework
[(see doc)]({% chapter_link host-configuration-doc.setup-your-build-host %})

Install Programs and Libraries you need
[(see doc)]({% chapter_link host-configuration-doc.getting-your-source-files %})

#### CANopen lib dependencies

Warning : this binding uses a specific version of liblely please use the one specified below

* Compile and Install lely-core CANopen library (preferred version 2.3.3):

``` bash
curl https://gitlab.com/lely_industries/lely-core/-/archive/v2.3.3/lely-core-v2.3.3.tar.gz
tar xf lely-core-v2.3.3.tar.gz
cd lely-core-v2.3.3
autoreconf -i
./configure --disable-python --disable-threads --prefix=$HOME/.local
make -j install
```

or from git

``` bash
git clone https://gitlab.com/lely_industries/lely-core.git
cd liblely
autoreconf -i
./configure --disable-python --disable-threads --prefix=$HOME/.local
make -j install
```

### CANopen Binding build

```bash
git clone https://github.com/redpesk-industrial/canopen-binding.git
cd canopen-binding
mkdir build
cd build
export PKG_CONFIG_PATH=$HOME/.local/lib64/pkgconfig:$HOME/.local/lib/pkgconfig:$PKG_CONFIG_PATH
cmake .. -DCMAKE_INSTALL_PREFIX=$HOME/.local
make
```
