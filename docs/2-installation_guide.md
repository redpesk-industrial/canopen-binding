# Installation

## Easy install

This binding is available in the Redpesk repo, so if you are running on one of the supported Linux distros you can install it with your package manager :

* Declare redpesk repository: [(see doc)](../../developer-guides/host-configuration/docs/1-Setup-your-build-host.html)

* Install the binding :

    ``` bash
    # Fedora / Redpesk :
    sudo dnf install canopen-binding

    # Ubuntu / Debian :
    sudo apt install canopen-binding

    # OpenSUSE :
    sudo zypper install canopen-binding
    ```

## Install from sources

### Dependencies

* Redpesk application framework 'afb-binder'
* Redpesk controller 'afb-libcontroller'
* Redpesk helpers 'afb-libhelpers'
* Redpesk cmake template 'afb-cmake-modules'
* lely-core CANopen library

> Note: _use *-dev or *-devel packages if available_

#### Redpesk dependencies

Add Redpesk repositories and install the Application Framework [(see doc)](../../developer-guides/host-configuration/docs/1-Setup-your-build-host.html)

Install Programs and Libraries you need [(see doc)](../../developer-guides/host-configuration/docs/2-getting-source-files.html)

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
