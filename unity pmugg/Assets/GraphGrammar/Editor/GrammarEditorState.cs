using System.Collections.Generic;
using UnityEditor;
using UnityEngine;

namespace Grammar {
    public class GrammarEditorState : ScriptableSingleton<GrammarEditorState> {
        [SerializeField]
        private string m_faceName = "Wall";

        [SerializeField]
        private int m_sides = 4;

        [SerializeField]
        private float m_angle = 90;

        [SerializeField]
        private bool m_excludeRepeats = false;

        public static string faceName {
            get { return instance.m_faceName; }
            set { instance.m_faceName = value; }
        }

        public static int sides {
            get { return instance.m_sides; }
            set { instance.m_sides = value; }
        }

        public static float angle {
            get { return instance.m_angle; }
            set { instance.m_angle = value; }
        }

        public static bool excludeRepeats {
            get { return instance.m_excludeRepeats; }
            set { instance.m_excludeRepeats = value; }
        }
    }
}