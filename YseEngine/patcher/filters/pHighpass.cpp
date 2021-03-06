#include "pHighpass.h"
#include "..\pObjectList.hpp"

using namespace YSE::PATCHER;

pObject * pHighpass::Create() { return new pHighpass(); }

pHighpass::pHighpass() {

  // in 0: audio buffer
  // in 1: frequency (float)

  // out 0: audio output

  inputs.emplace_back(PIN_TYPE::PIN_DSP_BUFFER, 0, this);
  inputs.emplace_back(PIN_TYPE::PIN_FLOAT, 1, this);
  outputs.emplace_back(PIN_TYPE::PIN_DSP_BUFFER, 0, this);

  inputs[1].SetData(1.f);
}

const char * pHighpass::Type() const {
  return YSE::OBJ::HIGHPASS;
}

void pHighpass::RequestData() {
  UpdateInputs();

  filter.setFrequency(inputs[1].GetFloat());

  if (inputs[0].IsConnected()) {
    outputs[0].SetData(&filter(*inputs[0].GetBuffer()));
  }
}