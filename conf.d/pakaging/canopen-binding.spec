%define debug_package %{nil}

Name: canopen-binding
Version: 0.1
Release: 15%{?dist}
Summary: canopen-binding is a binding that allows the control of a CANopen field network

License: No license to be set
URL: http://git.ovh.iot/redpesk/redpesk-industrial/canopen-binding
Source0: %{name}-%{version}.tar.gz

BuildRequires: afm-rpm-macros
BuildRequires: cmake
BuildRequires: afb-cmake-modules
BuildRequires: pkgconfig(json-c)
BuildRequires: pkgconfig(libsystemd) >= 222
BuildRequires: pkgconfig(afb-binding)
BuildRequires: pkgconfig(afb-libcontroller)
BuildRequires: pkgconfig(libmicrohttpd) >= 0.9.55
BuildRequires: pkgconfig(afb-libhelpers)
BuildRequires: pkgconfig(liblely-coapp)
BuildRequires: pkgconfig(lua) >= 5.3

%description
canopen-binding is a binding that allows the control of a CANopen field network from an Redpesk type system.
It handle different formats natively (int, float, string...) but can also handle custom formatting using plugins.
It is based on the opensource industrial c++ library Lely.

%define wgtname %{name}

Requires: liblely-coapp2

%prep
%autosetup -p 1

%files
%afm_files

%afm_package_test

%afm_package_devel

%build
%afm_configure_cmake
%afm_build_cmake

%install
%afm_makeinstall


%check

%clean
