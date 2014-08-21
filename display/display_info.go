package display

import "github.com/BurntSushi/xgb/randr"
import "sync"

type DisplayInfo struct {
	locker      sync.Mutex
	modes       map[randr.Mode]Mode
	outputNames map[string]randr.Output
	badOutputs  map[string]randr.Output
}

var GetDisplayInfo = func() func() *DisplayInfo {
	info := &DisplayInfo{
		modes:       make(map[randr.Mode]Mode),
		outputNames: make(map[string]randr.Output),
	}
	info.update()
	return func() *DisplayInfo {
		return info
	}
}()

func (info *DisplayInfo) ListNames() []string {
	var ret []string
	for name, _ := range info.outputNames {
		ret = append(ret, name)
	}
	return ret
}
func (info *DisplayInfo) ListOutputs() []randr.Output {
	var ret []randr.Output
	for _, op := range info.outputNames {
		ret = append(ret, op)
	}
	return ret
}

func (info *DisplayInfo) QueryModes(id randr.Mode) Mode {
	if op, ok := info.modes[id]; ok {
		return op
	} else {
		logger.Debug("can't find ", id)
		return Mode{}
	}
}
func (info *DisplayInfo) QueryOutputs(name string) randr.Output {
	if op, ok := info.outputNames[name]; ok {
		return op
	} else {
		logger.Debug("can't find ", name)
		return 0
	}
}

func isBadOutput(crtc randr.Crtc) bool {
	if crtc != 0 {
		cinfo, err := randr.GetCrtcInfo(xcon, crtc, LastConfigTimeStamp).Reply()
		if err != nil {
			return true
		}
		hasOnlyOneRotation := cinfo.Rotations == 1
		if hasOnlyOneRotation {
			return true
		}
	}
	return false
}
func (info *DisplayInfo) update() {
	info.locker.Lock()
	defer info.locker.Unlock()

	resource, err := randr.GetScreenResources(xcon, Root).Reply()
	if err != nil {
		logger.Error("GetScreenResouces failed", err)
		return
	}
	info.outputNames = make(map[string]randr.Output)
	info.badOutputs = make(map[string]randr.Output)
	for _, op := range resource.Outputs {
		oinfo, err := randr.GetOutputInfo(xcon, op, LastConfigTimeStamp).Reply()
		if err != nil {
			logger.Warning("DisplayInfo.update filter:", err)
			continue
		}
		if oinfo.Connection != randr.ConnectionConnected {
			continue
		}

		if isBadOutput(oinfo.Crtc) {
			info.badOutputs[string(oinfo.Name)] = op
			logger.Infof("detect a bad output[%s:%d], it wouldn't auto until user involved.", string(oinfo.Name), op)
		}
		info.outputNames[string(oinfo.Name)] = op
	}

	info.modes = make(map[randr.Mode]Mode)
	for _, minfo := range resource.Modes {
		info.modes[randr.Mode(minfo.Id)] = buildMode(minfo)
	}
}
