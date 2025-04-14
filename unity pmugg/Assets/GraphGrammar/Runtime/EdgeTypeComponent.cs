using System;
using System.Collections.Generic;
using System.Linq;
using UnityEngine;
using System.Runtime.InteropServices;

namespace Grammar {
    // Create a unique ID for each edge.
    public class EdgeMapper {
        int count;
        Dictionary<string, int> uniqueIds;

        public EdgeMapper() {
            count = 0;
            uniqueIds = new Dictionary<string, int>();
        }

        public int GetId(string name) {
            if (!uniqueIds.ContainsKey(name)) {
                uniqueIds.Add(name, count);
                count++;
            }
            return uniqueIds[name];
        }
    }

    // [AddComponentMenu("Grammar/Face Set")]
    public class EdgeTypeComponent : MonoBehaviour {
        // The same edge rotated to each side.
        public List<EdgeType> types = new List<EdgeType>();

        // [Tooltip("If this edge type is enabled.")]
        // public bool typeEnabled = true;

        [Tooltip("The minimum length.")]
        public float minLength = 0.2f;

        [Tooltip("The maximum length.")]
        public float maxLength = 4.0f;

        public string Export(EdgeMapper edgeMapper) {
            var eTypes = new List<int>();
            for (var i = 0; i < types.Count; i++) {
                eTypes.Add(edgeMapper.GetId(types[i].id));
            }
            return "<edgeBrush name='" + name +
                "' Min_Length='" + minLength +
                "' Max_Length='" + maxLength +
                "' types='[" + string.Join(",", eTypes) + "]'" +
                "></edgeBrush>";
        }
    }
}
