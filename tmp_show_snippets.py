from pathlib import Path

def print_function(path, needle, pad_before=6):
    lines = Path(path).read_text().splitlines()
    idx = next((i for i, line in enumerate(lines) if needle in line and not line.strip().endswith(';')), None)
    print(f"=== {path} ===")
    if idx is None:
        print(f"NOT FOUND: {needle}")
        return
    start = max(0, idx - pad_before)
    for i in range(start, idx + 1):
        print(f"{i + 1:6}: {lines[i]}")
    brace_count = 0
    started = False
    for i in range(idx, len(lines)):
        line = lines[i]
        if '{' in line:
            brace_count += line.count('{')
            started = True
        if '}' in line:
            brace_count -= line.count('}')
        if i > idx:
            print(f"{i + 1:6}: {lines[i]}")
        if started and brace_count == 0:
            break
    print()

for needle in [
    "void SetSpriteOamFlipBits(",
    "static void ApplyAffineMatrixFlips(",
]:
    print_function("src/sprite.c", needle)

for needle in [
    "void AdjustAffineSpriteOffset(",
    "void PlayerStaysShrunk(",
    "void StartPlayerShrinkAnimation(",
    "void ResetPlayerSprite(",
]:
    print_function("src/field_player_avatar.c", needle)
