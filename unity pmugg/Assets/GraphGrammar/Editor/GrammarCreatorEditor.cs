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
        private void OnEnable() {}

        public override void OnInspectorGUI() {
            if (GUILayout.Button("Editor")) {
                GrammarEditorWindow.Init();
            }
        }

        protected virtual void OnSceneGUI() {}
    }
}
