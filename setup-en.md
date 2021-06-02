# FreeBSD LKM Development VM Setup

> Translated from the Japanese version here: [`./setup-ja.md`](./setup-ja.md)

## Preface

When I was reading "Designing BSD Rootkits", I set up a virtual machine so I could follow along. In this file, I've recorded the steps to setting up a FreeBSD VM using Vagrant. At first it was going to be a note so I didn't forget, but I thought it might be useful to others so I decided to clean it and upload it.

## Virtual Machine

I'm using Vagrant to manage the virtual machine. Vagrant supports many Hypervisors (incl. VirtualBox and KVM), and takes care of most of the configuration for us. The VM itself is defined in a file called `Vagrantfile`.

```ruby
$provision_script = <<-SCRIPT
echo 'Downloading sources...'
fetch -qo /tmp https://download.freebsd.org/ftp/releases/amd64/12.2-RELEASE/src.txz
echo 'Extracting sources...'
tar Jxf /tmp/src.txz -C /
SCRIPT

Vagrant.configure("2") do |config|
  config.vm.box = "freebsd/FreeBSD-12.2-RELEASE"
  config.vm.synced_folder "./chapters", "/home/vagrant/chapters", 
    type: "rsync", rsync__args: ["-zz", "--delete", "--archive"]
  config.vm.provision "shell", inline: $provision_script
end
```

- `generic/freebsd12` is the [template](https://app.vagrantup.com/freebsd/boxes/FreeBSD-12.2-RELEASE) name
- `synced_folder` adds a shared folder. `rsync` is a personal preference
- `provision` is a script that is run when the VM is created. A variable is used here for readability

With the above settings, rsync will only sync in one direction. Changes made from inside the VM will be overwritten on the next sync, so please remember to only make changes on the host side. The advantage in this setup is that the development folders on the host will always stay clean, even if the build procedure generates many files on the VM. When this method is used, a command must be run after the VM is started to initiate the sync feature.

Basic Vagrant commands

- `vagrant up` Start VM 
- `vagrant halt` Power off the VM
- `vagrant reload` Reread the settings file and restart the VM
- `vagrant rsync-auto` Start rsync

※ `vagrant rsync-auto &` may be useful

## Verifying the dev environment

To verify the dev environment, try compiling and loading `chapters/01/hello_world/`. If there are no problems, the output should look something like the following:

```
vagrant@freebsd:~/chapters/01/hello_world % sudo kldload ./hello_world.ko
vagrant@freebsd:~/chapters/01/hello_world % sudo kldunload ./hello_world.ko
vagrant@freebsd:~/chapters/01/hello_world % dmesg | tail -n 2
[Addr: 0xffffffff82772119] Hello, world!
Goodbye, cruel world...
vagrant@freebsd:~/chapters/01/hello_world %
```
