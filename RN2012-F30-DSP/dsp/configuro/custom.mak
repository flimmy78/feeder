## THIS IS A GENERATED FILE -- DO NOT EDIT
.configuro: .libraries,e674 linker.cmd package/cfg/dsp_pe674.oe674

linker.cmd: package/cfg/dsp_pe674.xdl
	$(SED) 's"^\"\(package/cfg/dsp_pe674cfg.cmd\)\"$""\"/home/think/ti/syslink_2_21_01_05/examples/RN2012-F30-DSP/dsp/configuro/\1\""' package/cfg/dsp_pe674.xdl > $@
