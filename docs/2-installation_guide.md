# Installation

## Easy install

This binding is available in the redpesk repo, so if you are running on one of the supported Linux distros you can install it with your package manager :

* Declare redpesk repository: [(see doc)]({% chapter_link host-configuration-doc.setup-your-build-host %})

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

* redpesk application framework 'afb-binder'
* redpesk controller 'afb-libcontroller'
* redpesk helpers 'afb-libhelpers'
* redpesk cmake template 'afb-cmake-modules'
* lely-core CANopen library

> Note: _use *-dev or *-devel packages if available_

#### redpesk dependencies

Add redpesk repositories and install the Application Framework [(see doc)]({% chapter_link host-configuration-doc.setup-your-build-host %})

Install Programs and Libraries you need [(see doc)]({% chapter_link host-configuration-doc.getting-your-source-files %})

#### CANopen lib dependencies

Warning : this binding uses a specific version of liblely please use the one specified below

* Compile and Install lely-core CANopen library :

``` bash
git clone http://git.ovh.iot/redpesk/redpesk-industrial/liblely.git
cd liblely
autoreconf -i
./configure --disable-python --disable-threads
make
make install
```

### CANopen Binding build

```bash
git clone https://github.com/redpesk-industrial/canopen-binding.git
mkdir build && cd build
cmake ..
make
```
