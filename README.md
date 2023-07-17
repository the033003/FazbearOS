# FazbearOS
Please don't sue me scott

## Prebuilt ISOs
*Find downloads in Releases tab*

Requirements for prebuilt ISO:
- Virtual machine program (Qemu for Linux) (VMware for Windows)
- I have not been able to get VirtualBox to work, but feel free to try

Instructions for running:
- Emulate with Qemu: `qemu-system-x86_64 -cdrom PATH/TO/kernel.iso`  
OR 
- Burn to a usb and run on bare hardware

## Building from source
If you use this method, I'd highly reccommend using Linux.

Requirements:
- Docker (Installed AND Daemon running)
- Qemu x86_64 (VMware for Windows)
- VS Code or similar IDE to make changes (If desired)

Instructions:

- Prepare buildenv: `sudo docker build buildenv -t fazbear-buildenv`  

- Enter buildenv: `sudo docker run --rm -it -v "$(pwd)":/root/env fazbear-buildenv`  

- Build ISO with Make: `make build-x86_64` Then exit: `exit`  

- Emulate with Qemu: `qemu-system-x86_64 -cdrom dist/x86_64/kernel.iso`  

Done!

Other options for source:
- Remove docker image: `docker rmi fazbear-buildenv -f`
