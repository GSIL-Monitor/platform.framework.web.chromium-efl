Name:      org.tizen.dali_webview
Summary:   PUT SUMMARY HERE
Version:   0.1
Release:   1
Group:     TO_BE/FILLED_IN
License:   Apache
Source0:   %{name}-%{version}.tar.gz

BuildRequires: cmake
BuildRequires: pkgconfig(capi-appfw-application)
BuildRequires: pkgconfig(elementary)
BuildRequires: pkgconfig(capi-system-system-settings)
BuildRequires: pkgconfig(libtbm)
BuildRequires: pkgconfig(dali-core)
BuildRequires: pkgconfig(dali-toolkit)
BuildRequires: pkgconfig(dali-adaptor)
BuildRequires: pkgconfig(chromium-efl)

%define _pkgdir %{TZ_SYS_RO_APP}/%{name}
%define _bindir %{_pkgdir}/bin

%description
PUT DESCRIPTION HERE

%prep
%setup -q

%build
echo "pkgdir: %{_pkgdir}"
echo "path: %{_manifestdir}"

cmake \
        -DCMAKE_INSTALL_PREFIX=%{_pkgdir} \
        -DPACKAGE_NAME=%{name} \
        -DBINDIR=%{_bindir}

make %{?jobs:-j%jobs} -lpthread
%install
%make_install
install --directory %{buildroot}/%{_datadir}

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root,-)
%{_bindir}/*
