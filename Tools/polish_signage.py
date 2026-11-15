import unreal as u
l=u.get_editor_subsystem(u.LevelEditorSubsystem)
a=u.get_editor_subsystem(u.EditorActorSubsystem)
for state in ('Normal','Shift'):
    l.load_level('/Game/ShiftStore/Maps/'+state)
    for x in a.get_all_level_actors():
        name=x.get_actor_label()
        if name in ('Aisle01','Aisle02','Aisle03'):
            col=int(name[-1])-1
            x.set_actor_location(u.Vector(400+col*400+(30 if state=='Shift' and col==1 else 0),445.7,166.5),False,False)
            x.get_component_by_class(u.TextRenderComponent).set_world_size(7.5)
    l.save_current_level()
