# FazbearOS
Please don't sue me scott

## Prebuilt ISOs
*Find downloads in Releases tab*

Requirements for prebuilt ISO:
- Virtual machine program (Qemu for Linux ) (Virtualbox for Windows/OS X)

Instructions for running:
- Emulate with Qemu: `qemu-system-x86_64 -cdrom PATH/TO/kernel.iso`  
OR 
- Burn to a usb and run on bare hardware (UNTESTED)

## Building from source
If you use this method, I'd highly reccommend using Linux. Windows is untested.

Requirements:
- Docker (Installed AND Daemon running)
- Qemu x86_64 (Or Virtualbox for Windows/OS X)
- VS Code or similar IDE to make changes (If desired)

Instructions:

- Prepare buildenv: `docker build buildenv -t -buildenv fazbear-buildenv`  

- Enter buildenv: `docker run --rm -it -v "$(pwd)":/root/env fazbear-buildenv`  

- Build ISO with Make: `make build-x86_64` Then exit: `exit`  

- Emulate with Qemu: `qemu-system-x86_64 -cdrom dist/x86_64/kernel.iso`  

Done!

Other options for source:
- Remove docker image: `docker rmi fazbear-buildenv -f`
