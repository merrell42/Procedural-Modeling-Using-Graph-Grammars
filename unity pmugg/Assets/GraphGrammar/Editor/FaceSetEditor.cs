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
    [CustomEditor(typeof(FaceSet))]
    public class FaceSetEditor : Editor {
        private void OnSceneGUI() {}
    }
}