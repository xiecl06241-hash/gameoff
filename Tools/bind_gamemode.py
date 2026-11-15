import unreal as u

gm = u.load_class(None, '/Script/ShiftStore.ShiftStoreGameMode')
if not gm:
    raise RuntimeError('Could not load /Script/ShiftStore.ShiftStoreGameMode')

levels = u.get_editor_subsystem(u.LevelEditorSubsystem)
editor = u.get_editor_subsystem(u.UnrealEditorSubsystem)

for name in ('Normal', 'Shift'):
    path = f'/Game/ShiftStore/Maps/{name}'
    levels.load_level(path)
    world = editor.get_editor_world()
    world.get_world_settings().set_editor_property('default_game_mode', gm)
    levels.save_current_level()
    u.log(f'SHIFTSTORE_GAMEMODE_BOUND: {path}')
