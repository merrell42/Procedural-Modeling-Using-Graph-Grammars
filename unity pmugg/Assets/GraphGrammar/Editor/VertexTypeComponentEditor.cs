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
    [CustomEditor(typeof(VertexTypeComponent))]
    public class VertexTypeComponentEditor : Editor {
        private void OnSceneGUI() {
            VertexTypeComponent component = (VertexTypeComponent)target;

            Event currentEvent = Event.current;
            if (currentEvent.type == EventType.KeyDown) {
                if (currentEvent.keyCode == KeyCode.Space) {
                    component.typeEnabled = !component.typeEnabled;
                    var obj = component.gameObject;
                    var meshRenderer = obj.GetComponent<MeshRenderer>();
                    Material[] materials = meshRenderer.sharedMaterials.Clone() as Material[];
                    materials[1] = component.typeEnabled ? Resources.Load<Material>("enabled") : Resources.Load<Material>("disabled");
                    meshRenderer.materials = materials;
                }
            }
        }
    }
}