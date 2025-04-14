using System;
using System.IO;
using System.Collections.Generic;
using System.Linq;
using System.Xml.Serialization;
using UnityEngine;

namespace Grammar {
    /* [System.Serializable]
    [XmlRoot("vDecoration")]
    public class vDecoration {
        [XmlAttribute("name")]
        public string name { get; set; }
        [XmlAttribute("types")]
        public int[] types;
        [XmlAttribute("desirability")]
        public float desirability { get; set; }
    } */

    // [AddComponentMenu("Grammar/Face Set")]
    public class VertexTypeComponent : MonoBehaviour {

        // The same vertex rotated to each side.
        public List<VertexType> types = new List<VertexType>();
        // public VertexType type;

        [Tooltip("If this vertex type is enabled.")]
        public bool typeEnabled = true;

        [Tooltip("How desirable this vertex is.")]
        public float desirability = 0;

        public string Export() {
            /* var obj = new vDecoration();
            obj.name = name;
            obj.desirability = desirability;

            var vTypes = new List<int>();
            for (var i = 0; i < types.Count; i++) {
                vTypes.Add(types[i].id);
            }
            obj.types = vTypes.ToArray();
            XmlSerializer serializer = new XmlSerializer(typeof(vDecoration));
            StringWriter stringWriter = new StringWriter();
            serializer.Serialize(stringWriter, obj); */

            var vTypes = new List<int>();
            for (var i = 0; i < types.Count; i++) {
                vTypes.Add(types[i].id);
            }
            return "<vDecoration name='" + name +
                "' Desirability='" + desirability +
                "' types='[" + string.Join(",", vTypes) + "]'>" +
                "</vDecoration>";
        }
    }
}