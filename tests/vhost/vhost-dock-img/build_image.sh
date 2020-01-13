#cp ../../../../dpdk/x86_64-native-linuxapp-gcc/app/testpmd .
cp ../vhost-srv.sh .
cp ../../../tests-srv .
sudo docker image build -t dpdk-app-testpmd:latest .
