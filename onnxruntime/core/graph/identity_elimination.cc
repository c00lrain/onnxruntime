#include "core/graph/rewrite_rule.h"
#include "core/graph/identity_elimination.h"
#include "core/graph/graph.h"
#include "core/graph/op.h"
#include "core/common/logging/logging.h"

namespace onnxruntime {

Status EliminateIdentity::Apply(GraphEditor* graph_editor, Node* node, bool* modified) {
  std::map<const NodeArg*, NodeArg*> replacement_defs;
  auto id_input = node->InputDefs()[0];
  auto id_output = node->OutputDefs()[0];
  replacement_defs[id_output] = const_cast<NodeArg*>(id_input);

  // Replace (input) defs of the nodes following the Identity with the input to the Identity.
  for (auto it = node->OutputNodesBegin(); it != node->OutputNodesEnd(); it++) {
    const_cast<Node*>(*it)->ReplaceDefs(replacement_defs);
    *modified = true;
  }

  // Remove the Identity node.
  graph_editor->RemoveNode(node->Index());

  // TODO: Make sure resolve is not required here.
  //LOTUS_RETURN_IF_ERROR(graph_editor->Resolve());

  return Status::OK();
}

bool EliminateIdentity::SatisfyCondition(const Node& node) {
  (void)node;
  return true;  // No additional condition required.
}

}  // namespace onnxruntime