#!/bin/sh

P2P_PATH=${PWD}
cp $P2P_PATH/bin/IOTDaemon $P2P_PATH/../../bin/resource/3518e200_P3_common/new_p2p_tmp -apv
chmod 777 $P2P_PATH/../../bin/resource/3518e200_P3_common/new_p2p_tmp/IOTDaemon
cp $P2P_PATH/lib/*.so $P2P_PATH/../../bin/resource/3518e200_P3_common/new_p2p_tmp/lib -apv
cp $P2P_PATH/include/p2psdk.h $P2P_PATH/../../src/p2p
cp $P2P_PATH/lib/*.so $P2P_PATH/../../lib/arm-hisiv300-linux- -apv

#cp bin/IOTDaemon ../../bin/resource/3518e200_P3_common/new_p2p_tmp -apv
#chmod 777 ../../bin/resource/3518e200_P3_common/new_p2p_tmp/IOTDaemon
#cp lib/* ../../bin/resource/3518e200_P3_common/new_p2p_tmp/lib -apv
#cp include/p2psdk.h ../../src/p2p
#cp lib/* ../../lib/arm-hisiv300-linux- -apv
