# ksm [![Build Status](https://travis-ci.org/asamy/ksm.svg?branch=master)](https://travis-ci.org/asamy/ksm)
<a href="https://scan.coverity.com/projects/asamy-ksm">
  <img alt="Coverity Scan Build Status"
       src="https://scan.coverity.com/projects/10823/badge.svg"/>
</a>

A really simple and lightweight x64 hypervisor written in C for Intel processors.

## Features

- IDT Shadowing
- EPT violation #VE (if not available natively, it will keep using VM-Exit instead)
- EPTP switching VMFUNC (if not available natively, it will be emulated using a VMCALL)

## Why not other hypervisors?

You may have already guessed from the `Features` part, if not, here are some reasons:

- Do not implement the new processor features KSM implements (VMFUNC, #VE, etc.)
- Are not simple enough to work with or understand
- Simply, just have messy code base or try too hard to implement endless C++ features that just make code ugly.
- Too big code base and do not have the same purpose (e.g. research or similar)

Such features for such purpose is really crucial, for my purpose, I wanted a quicker physical memory virtualization
technique that I can relay on.

## Requirements

- An Intel processor
- A working C compiler (GCC or CLang or Microsoft compiler (CL)).  You can use VS 2015.

## Compiling under MinGW

Simply `make C=1` (if cross compiling under Linux) or `mingw32-make` (under MinGW).

## Unsupported features (hardware, etc.)

- UEFI
- Intel TXT
- VT-x nesting (i.e. having a vm running inside it not the other way around!)

## Debugging and/or testing

Since #VE and VMFUNC are now optional and will not be enabled unless the CPU support it, you can now test under VMs with
emulation for VMFUNC.

## Supported Kernels

All x64 NT kernels starting from the Windows 7 NT kernel.  It was mostly tested under Windows 7/8/8.1/10.

## Porting to other kernels (e.g. Linux or similar) guidelines

- Port `mm.h` functions (`mm_alloc_pool, mm_free_pool, __mm_free_pool`).  You'll need `__get_free_page` instead of `ExAllocatePool`.
- Port `acpi.c` (not really needed) for re-virtualization on S1-3 or S4 state (commenting it out is OK).
- Port `main.c` for some internal windows stuff, e.g. `DriverEntry`, etc.  Perhaps even rename to something like main_windows.c or similar.
- Port `page.c` for the hooking example (not required, but it's essential to demonstrate usage).

Hopefully didn't miss something important, but these are definitely the mains.

## Contributions

Contributions are really appreciated and can be submitted by one of the following:

- Patches (e-mail)
- Github pull requests
- git request-pull

It'd be appreciated if you use a separate branch for your submissions (other than master, that is).

## TODO / In consideration

- APIC virtualization (Partially implemented)
- MMIO (Partially implemented)
- UEFI support
- Intel TXT support
- Nesting support (Partially implemented)
- Interrupt queueing (currently if an injection fails, it will just ignore it, should be simple).

## Loading the driver

In commandline as administrator:

1. `sc create ksm type= kernel binPath= C:\path\to\your\ksm.sys`
2. `sc start ksm`

Unloading:
`sc stop ksm`

Note: KSM is provided unsigned, that means, you might need to enable Testsigning to be able to load the driver.  KSM does not have a binary distributed publicly, you'll have to compile it yourself.
You can also use [kload](https://github.com/asamy/kload)

## Technical information

Note: If the processor does not support VMFUNC or #VE, they will be disabled and instead, emulated via VM-exit.

### IDT shadowing

- By enabling the descriptor table exiting bit in processor secondary control, we can easily establish this
- On initial startup, we allocate a completely new IDT base and copy the current one in use to it (also save the old
												   one)
- When a VM-exit occurs with an `EXIT_REASON_GDT_IDT_ACCESS`, we simply just give them the cached one (on sidt) or (on
														  lidt),
	we copy the new one's contents, discarding the hooked entries we know about, thus not letting them know about
	our stuff.

### #VE setup and handling

We use 3 EPT pointers, one for executable pages, one for readwrite pages, and last one for normal usage.  (see next
													   section)

- `vcpu.c`: in `setup_vmcs()` where we initially setup the VMCS fields, we then set the relevant fields (`VE_INFO_ADDRESS`,
													`EPTP_LIST_ADDRESS`,
													`VM_FUNCTION_CTL`) and enable
relevant bits VE and VMFUNC in secondary processor control.

- `x64.asm` (or `x64.S` for GCC): which contains the `#VE` handler (`__ept_violation`) then does the usual interrupt handling and then calls
	`__ept_handle_violation` (`ept.c`) where it actually does what it needs to do.
- `ept.c`: in `__ept_handle_violation` (`#VE` handler *not* `VM-exit`), usually the processor will do the `#VE` handler instead of
	the VM-exit route, but sometimes it won't do so if it's delivering another exception.  This is very rare.
- `ept.c`: while handling the violation via `#VE`, we call `vmfunc` only when we detect that the faulting address is one of
	our interest (e.g. a hooked page), then we determine which `EPTP` we want and execute `VMFUNC` with that EPTP index.

### Hooking executable pages

#### Execute-only EPT for executable page hooking, RW for read or write access

	(... to avoid a lot of violations, we just mark the page as execute only and replace the _final_ page frame
	 number so that it just goes straight ahead to our trampoline)
Since we use 3 EPT pointers, and since the page needs to be read and written to sometimes (e.g. patchguard
											   verification),
      we also need to catch RW access to the page and then switch the EPTP appropriately according to
      the access.  In that case we switch over to `EPTP_RWHOOK` to allow RW access only!
	The third pointer is used for when we need to call the original function.

## Enabling certain features / tests

You can define one or more of the following:

- `RUN_TEST` - Runs a small MmMapIoSpace shadow hook test.
- `ENABLE_PML` - Enables Page Modification Log if supported.
- `EMULATE_VMFUNC` - Forces emulation of VMFUNC even if CPU supports it.
- `EPT_SUPPRESS_VE` - Force suppress VE bit in EPT.
- `ENABLE_ACPI` - Enable S1-3-S4 power state monitoring for re-virtualization
- `NESTED_VMX` - Enable experimental VT-x nesting

## Reporting bugs (or similar)

You can report bugs by using Github issues, please provide the following:

- System information (version including build number, CPU information perhaps codename too)
- The git tree hash
- Anything else you feel is relevant

If it's a crash, please provide the following:

- A minidump (C:\windows\minidump) or a memory dump (C:\windows\memory.dmp).  Former prefered.
- The compiled .sys and the .pdb/.dbg file
- The Kernel executable if possible, e.g. ntoskrnl.exe from C:\Windows\System32

## Thanks to...

- Linux kernel (KVM)
- HyperPlatform

## License

GPL v2 firm, see LICENSE file.

```
    ksm - a really simple and fast x64 hypervisor
    Copyright (C) 2016 Ahmed Samy <f.fallen45@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
```