using UnityEditor;
using UnityEngine;

namespace Grammar {
    public static class GrammarSceneView {
        private const float ColumnSpacing = 1.8f;
        private const float RowSpacing = 1.8f;
        private static readonly string[] CategoryLabels = { "Rules", "Starter Rules", "Ground Rules" };

        public static void Show(string grammarName) {
            ClearExisting();

            var root = new GameObject("Grammar View");
            int row = 0;

            for (int category = 0; category < CategoryLabels.Length; category++) {
                int ruleCount = GrammarDllMesh.getNumProductionRules(category);
                for (int ruleIndex = 0; ruleIndex < ruleCount; ruleIndex++) {
                    int graphCount = GrammarDllMesh.getProductionRuleGraphCount(category, ruleIndex);
                    for (int graphIndex = 0; graphIndex < graphCount; graphIndex++) {
                        var meshData = GrammarDllMesh.getProductionRuleGraphMesh(category, ruleIndex, graphIndex);
                        string objectName = $"{CategoryLabels[category]} {ruleIndex}.{graphIndex}";
                        var graphObject = GrammarDllMesh.BuildGameObject(meshData, objectName);
                        graphObject.transform.SetParent(root.transform, false);
                        graphObject.transform.localPosition = new Vector3(graphIndex * ColumnSpacing, 0f, -row * RowSpacing);
                    }
                    row++;
                }
            }

            Selection.activeGameObject = root;
            if (SceneView.lastActiveSceneView != null) {
                SceneView.lastActiveSceneView.FrameSelected();
            }
        }

        private static void ClearExisting() {
            var existing = GameObject.Find("Grammar View");
            if (existing != null) {
                Object.DestroyImmediate(existing);
            }
        }
    }
}
