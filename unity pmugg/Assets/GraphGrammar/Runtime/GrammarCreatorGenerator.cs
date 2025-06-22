using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using UnityEngine;
using UnityEngine.Profiling;
using UnityEngine.Serialization;
using UnityEngine.Tilemaps;

namespace Grammar {
    /* [Serializable]
    public class TileList
    {
        public List<GrammarTileBase> tiles;
    } */

    [AddComponentMenu("Grammar/Grammar Creator")]
    public class GrammarCreator : MonoBehaviour {
        [Tooltip("The name of the new face set.")]
        public string faceName = "Wall";

        [Tooltip("The number of different sides to the new face set.")]
        public int sides = 4;

        [Tooltip("The angle with respect to the ground. 0 is parallel to the ground. 90 is vertical.")]
        public float angle = 90;
    }
}
