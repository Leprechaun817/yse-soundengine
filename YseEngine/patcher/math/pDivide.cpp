#include "pDivide.h"
#include "..\pObjectList.hpp"

using namespace YSE::PATCHER;

pObject * pDivide::Create() { return new pDivide(); }

pDivide::pDivide() {

  // in 0: audio buffer
  // in 1: multiplier (float or audio)

  // out 0: audio output

  inputs.emplace_back(PIN_TYPE::PIN_DSP_BUFFER, 0, this);
  inputs.emplace_back(PIN_TYPE::PIN_FLOAT | PIN_TYPE::PIN_DSP_BUFFER, 1, this);
  outputs.emplace_back(PIN_TYPE::PIN_DSP_BUFFER, 0, this);

  inputs[1].SetData(1.f);
}

const char * pDivide::Type() const {
  return YSE::OBJ::DIVIDE;
}

void pDivide::RequestData() {
  UpdateInputs();

  if (inputs[0].IsConnected()) {
    output = *inputs[0].GetBuffer();
  }

  if (inputs[1].GetCurentDataType() == PIN_FLOAT) {
    output /= inputs[1].GetFloat();
  }
  else if (inputs[1].GetCurentDataType() == PIN_DSP_BUFFER) {
    output /= *inputs[1].GetBuffer();
  }

  outputs[0].SetData(&output);
}