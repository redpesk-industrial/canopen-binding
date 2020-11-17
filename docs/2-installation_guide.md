# install

## Easy install

This binding is available in the Redpesk repo, so if you are runing a Redpesk or a redhat based os you can install it with package manager :

``` bash
# Add the repository
cat << EOF > /etc/yum.repos.d/canopen-binding.repo
[canopen-binding]
name=canopen-binding
baseurl=https://iotbzh-app.redpesk.bzh/kbuild/repos/canopen-binding--redpesk-devel-28-build/latest/\$basearch
enabled=1
repo_gpgcheck=0
type=rpm
gpgcheck=0
skip_if_unavailable=False
EOF

# Install the binding
sudo dnf install canopen-binding
```

## Install from sources

### Dependencies

* Redpesk application framework 'afb-binder'
* Redpesk controller 'afb-libcontroller-devel'
* Redpesk helpers 'afb-libhelpers-devel'
* Redpesk cmake template 'afb-cmake-modules'
* lely-core CANopen library

#### Redpesk dependencies

* Redpesk microservices build tools: [(see doc)](https://docs.iot.bzh/docs/en/master/devguides/reference/2-download-packages.html)
* Install Redpesk controller: [(see doc)](https://docs.iot.bzh/docs/en/guppy/devguides/reference/ctrler/controller.html)

#### CANopen lib dependencies

* Compile and Install fd_loop brach of lely-core CANopen library :

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
mkdir build && cd build
cmake ..
make
```
