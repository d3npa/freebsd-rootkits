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
