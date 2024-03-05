# RPM sgpec file
# This file is used to build Redhat Package Manager packages
# Such packages make it easy to install and uninstall
# the library and related files from binaries or source.
#
# RPM. To build, use the command: rpmbuild --clean -ba harbour-pikefight.spec
#

Name: harbour-pikefight

# Harbour requirements.
%define __provides_exclude_from ^%{_datadir}/.*$
%define __requires_exclude ^libjpeg.*|libcairo.*|libpng15.*|libpsl.*|libdconf.*|libicui18n.*|libsqlite3.*|libpixman-1.*|libfreetype.*|libicuuc.*|libicudata.*|libc.*$
%{!?qtc_qmake5:%define qtc_qmake5 %qmake5}
%{!?qtc_make:%define qtc_make make}
%{?qtc_builddir:%define _builddir %qtc_builddir}

Summary: Pike with Map
Version: 1.4.4
Release: 1%{?dist}
Group: Applications/Engineering
License: GPLv2
Source: %{name}-%{version}.tar.gz

BuildRoot: %{_tmppath}/%{name}-%{version}-buildroot
Requires: sailfishsilica-qt5
Requires: mapplauncherd-booster-silica-qt5
Requires: qt5-qtdeclarative-import-positioning
Requires: qt5-qtdeclarative-import-folderlistmodel
Requires: nemo-qml-plugin-notifications-qt5
Requires:   sailfishsilica-qt5 >= 0.10.9
BuildRequires: pkgconfig(qdeclarative5-boostable)
BuildRequires: pkgconfig(sailfishapp) >= 1.0.2
BuildRequires: pkgconfig(Qt5Core)
BuildRequires: pkgconfig(Qt5Qml)
BuildRequires: pkgconfig(Qt5Quick)
BuildRequires: pkgconfig(Qt5Multimedia)
BuildRequires: pkgconfig(Qt5Svg)
BuildRequires: pkgconfig(qt5embedwidget)
BuildRequires: desktop-file-utils
BuildRequires: pkgconfig(Qt5Positioning)
BuildRequires: pkgconfig(gobject-2.0)
BuildRequires: pkgconfig(cairo)
#BuildRequires: pkgconfig(libpng15)
BuildRequires: pkgconfig(dconf)
BuildRequires: pkgconfig(libxml-2.0)
BuildRequires: pkgconfig(libcurl)
BuildRequires: libjpeg-turbo-devel

%description
Pike Fight Uses Maep. Maep is a tile based map utility for services like OpenStreetMap, Google maps
and Virtual earth.


%prep
rm -rf $RPM_BUILD_ROOT
%setup -q -n %{name}-%{version}

%build
%qtc_qmake5
%qtc_make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%qmake5_install
# Copy here the blacklisted libraries

install -d %{buildroot}/usr/share/%{name}/lib
install -m 644 -p %{_libdir}/libjpeg.so.62 %{buildroot}/usr/share/%{name}/lib/
install -m 644 -p %{_libdir}/libcairo.so.2 %{buildroot}/usr/share/%{name}/lib/
install -m 644 -p %{_libdir}/libdconf.so.1 %{buildroot}/usr/share/%{name}/lib/
install -m 644 -p %{_libdir}/libsqlite3.so.0 %{buildroot}/usr/share/%{name}/lib/
install -m 644 -p %{_libdir}/libpixman-1.so.0 %{buildroot}/usr/share/%{name}/lib/
install -m 644 -p %{_libdir}/libfreetype.so.6 %{buildroot}/usr/share/%{name}/lib/
install -m 644 -p %{_libdir}/libicui18n.so.68 %{buildroot}/usr/share/%{name}/lib/
install -m 644 -p %{_libdir}/libicuuc.so.68 %{buildroot}/usr/share/%{name}/lib/
install -m 644 -p %{_libdir}/libicudata.so.68 %{buildroot}/usr/share/%{name}/lib/
install -m 644 -p %{_libdir}/libpsl.so %{buildroot}/usr/share/%{name}/lib/
%files
%defattr(644,root,root,-)
#/usr/share/applications
#/usr/share/icons
#/usr/bin

# eg /usr/share/applications/harbour-tripometer.desktop
%defattr(644,root,root,-)
%attr(755,-,-) %{_bindir}
%{_datadir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/*/apps/%{name}.png




