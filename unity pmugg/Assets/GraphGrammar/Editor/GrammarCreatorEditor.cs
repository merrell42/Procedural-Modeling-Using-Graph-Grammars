using UnityEditor;
using UnityEngine;
using System;
using UnityEditorInternal;
using UnityEditor.IMGUI.Controls;
using System.Threading.Tasks;
using System.Threading;
using System.Linq;
using System.Collections.Generic;

namespace Grammar {
    [CustomEditor(typeof(GrammarCreator))]
    public class GrammarCreatorEditor : Editor {
        private const string faceGroupName = "Faces";
        private const string edgeGroupName = "Edges";
        private const string vertexGroupName = "Vertices";

        private void CreateChild(string childName) {
            var creator = target as GrammarCreator;
            if (!creator.transform.Find(childName)) {
                GameObject faceGroup = new GameObject();
                faceGroup.name = childName;
                faceGroup.transform.parent = creator.transform;
            }
        }

        private void OnEnable() {
            CreateChild(faceGroupName);
            CreateChild(edgeGroupName);
            CreateChild(vertexGroupName);
            var creator = target as GrammarCreator;
            creator.tag = "Grammar";
        }

        public override void OnInspectorGUI() {
            if (GUILayout.Button("Editor")) {
                GrammarEditorWindow.Init();
            }
        }

        /* private bool ShouldRecordUndo(GrammarCreator creator) {
            return creator.recordUndo && creator.surfaceMesh == null;
        } */

        protected virtual void OnSceneGUI() {}
    }
}