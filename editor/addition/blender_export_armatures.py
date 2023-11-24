import bpy
import json
from pathlib import Path

from mathutils import * 

import os
clear = lambda: os.system('cls')
clear()

output_path = "E:\\Repos\\WGdotTest\\" # change

output_name = "_armatures" # change

json_output_path = Path(str(output_path))

id = 0

for ob in bpy.data.objects:
    print(ob.type)
    Armatures = {}
    
    if ob.type == 'ARMATURE':
        armature = ob
        bones = []
        bid = 0
        for bone in armature.pose.bones:
            print(bone.name)
            bm = bone.matrix
            bn = {}
            
            bn["name"] = bone.name
#            bn["id"] = bid
            bn["parent"] = -1
            pbid = 0
            if bone.parent is not None:
                for bone_2 in armature.pose.bones:
                    
                    if bone.parent == bone_2:
                        bn["parent"] = pbid
                        break
                    pbid += 1
            
            bid += 1
            if bn["parent"] != -1:
                tr = bone.parent.bone.matrix_local.inverted() * bone.bone.matrix_local
                bn["transform_mat"] = []
                for i in range(4):
                    t = []
                    for j in range(4):
                        t.append(1 if i == j else 0)
                    bn["transform_mat"].append(t)
                
                t = bone.head - bone.parent.head
                bn["transform_mat"][3][0] = t.x
                bn["transform_mat"][3][1] = t.y
                bn["transform_mat"][3][2] = t.z
            else:
                bn["transform_mat"] = []
                for i in range(4):
                    t = []
                    for j in range(4):
                        t.append(1 if i == j else 0)
                    bn["transform_mat"].append(t)
                

            bones.append(bn)
            
        with open(output_path / Path(output_name + "." + str(id) + "_" + ob.name  + ".bone.json"), "w") as out_file:
            out_file.write(json.dumps(bones, indent=1))



