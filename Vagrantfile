Vagrant.configure("2") do |config|
  config.vm.box = "generic/freebsd12"
  config.vm.synced_folder "./", "/vagrant", type: "rsync",
    rsync__args: ["-zz", "--delete", "--archive" ]
end
