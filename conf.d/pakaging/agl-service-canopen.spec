%define debug_package %{nil}

Name: agl-service-canopen
Version: 0.1
Release: 13%{?dist}
Summary: agl-service-canopen summary

License: No license to be set
URL: http://git.ovh.iot/jgautier/agl-service-canopen.git
Source0: %{name}-%{version}.tar.gz

BuildRequires: afm-rpm-macros
BuildRequires: cmake
BuildRequires: cmake-apps-module
BuildRequires: pkgconfig(json-c)
BuildRequires: pkgconfig(libsystemd) >= 222
BuildRequires: pkgconfig(afb-daemon)
BuildRequires: pkgconfig(appcontroller)
BuildRequires: pkgconfig(libmicrohttpd) >= 0.9.55
BuildRequires: pkgconfig(afb-helpers)
BuildRequires: pkgconfig(liblely-coapp)
BuildRequires: pkgconfig(lua) >= 5.3

%description
agl-service-canopen is a binding that allows the control of a CANopen field network from an AGL type system.
It handle different formats natively (int, float, string...) but can also handle custom formatting using plugins.
It is based on the opensource industrial c++ library Lely.

%define wgtname %{name}

%define afm_widget_requires \
Requires: liblely-coapp2

%afm_package_widget
%afm_package_widget_test

%prep
%autosetup -p 1


%build
%afm_configure_cmake_release
%afm_build_cmake
%afm_widget

%install
%afm_install_widget
%afm_install_widgettest

%check

%clean
