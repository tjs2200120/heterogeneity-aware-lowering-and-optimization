//===- dequant.cc --------------------------------------------------------===//
//
// Copyright (C) 2019-2021 Alibaba Group Holding Limited.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// =============================================================================

#include <string>

#include "halo/lib/ir/common_instructions.h"
#include "halo/lib/target/generic_cxx/generic_cxx_codegen.h"

namespace halo {

void GenericCXXCodeGen::RunOnInstruction(HgDequantInst* inst) {
  const Def& input = inst->GetOperand(0);

  CXXValue op0 = ir_mapping_[input];
  CXXValue ret(inst->GetName(), op0.type);

  const std::string& enum_ns_layout = "odla_memory_layout::";
  const std::string& enum_prefix = "ODLA_";
  const std::string& enum_ns =
      opts_.dialect == Dialect::CXX_11 ? enum_ns_layout : "";

  int is_per_channel = inst->GetIsPerChannel();
  const std::string& input_layout =
      enum_ns + enum_prefix +
      (inst->GetInDataFormat() == "NHWC" ? "CHANNELS_LAST" : "CHANNELS_FIRST");
  const std::string& output_layout =
      enum_ns + enum_prefix +
      (inst->GetOutDataFormat() == "NHWC" ? "CHANNELS_LAST" : "CHANNELS_FIRST");

  // Todo : transpose support, per_channel support, int16/fp16 support
  HLCHECK(input_layout == output_layout);
  HLCHECK(is_per_channel == false);

  std::vector<CXXValue> inputs;
  for (size_t i = 0; i < inst->GetNumOfOperands(); ++i) {
    const Def& op = inst->GetOperand(i);
    CXXValue op_v = ir_mapping_[op];
    inputs.push_back(op_v);
  }

  std::vector<CXXValue> rets;
  rets.emplace_back(inst->GetName(),
                    TensorTypeToCXXType(inst->GetResultType(0), false));
  ir_mapping_[Def(inst, 0)] = rets[0];

  // construct the odla_value_ids string
  unsigned int id = 0;
  std::ostringstream os;
  os << "{.size = " << rets.size() << ", .value_ids = {";
  for (auto& one : rets) {
    os << "(const odla_value_id)";
    EmitODLAVauleId(one, os);
    if (++id != rets.size()) {
      os << ", ";
    }
  }
  os << "}}";

  EmitODLACall<2, false>(rets, "odla_CustomOp", inputs, "\"HgaiDequant\"",
                         "\"" + inst->GetName() + "\"", os.str(), input_layout,
                         output_layout, is_per_channel);
}

} // namespace halo
