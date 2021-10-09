#!/bin/sh
KO="dist_comm.ko"
text=$(< /sys/module/dist_comm/sections/.text)
bss=$(< /sys/module/dist_comm/sections/.bss)
data=$(< /sys/module/dist_comm/sections/.data)
echo "text: ""$text"
echo "bss: ""$bss"
echo "data: ""$data"
echo "add-symbol-file ./dist_comm.ko $text -s .data $data -s .bss $bss"