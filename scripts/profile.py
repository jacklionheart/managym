#!/usr/bin/env python3
"""
Profile managym simulation throughput.
Grey Ogre mirror match, random policy, single thread.
"""

import argparse
import time
import random
import re
import managym


def parse_profiler_stats(profiler_dict):
    """Parse profiler string output into structured data."""
    result = {}
    for path, stat_str in profiler_dict.items():
        # Parse "total=0.123456s, count=100"
        total_match = re.search(r'total=([0-9.e+-]+)s', stat_str)
        count_match = re.search(r'count=(\d+)', stat_str)
        if total_match and count_match:
            result[path] = {
                "total_time": float(total_match.group(1)),
                "count": int(count_match.group(1)),
            }
    return result


def run_profile(games: int, seed: int):
    """Run the profiling benchmark."""
    random.seed(seed)

    # Grey Ogre mirror: 20 Mountains + 20 Grey Ogres each
    hero_deck = {"Mountain": 20, "Grey Ogre": 20}
    villain_deck = {"Mountain": 20, "Grey Ogre": 20}

    hero_config = managym.PlayerConfig("Hero", hero_deck)
    villain_config = managym.PlayerConfig("Villain", villain_deck)
    configs = [hero_config, villain_config]

    env = managym.Env(seed=seed, skip_trivial=True, enable_profiler=True, enable_behavior_tracking=True)

    total_steps = 0
    start_time = time.perf_counter()

    for game_idx in range(games):
        obs, info = env.reset(configs)
        done = False

        while not done:
            # Random action from available
            num_actions = len(obs.action_space.actions)
            action = random.randint(0, num_actions - 1) if num_actions > 0 else 0
            obs, reward, terminated, truncated, info = env.step(action)
            done = terminated or truncated
            total_steps += 1

    end_time = time.perf_counter()
    wall_time = end_time - start_time

    # Get info including profiler data
    final_info = env.info()

    # Calculate metrics
    games_per_sec = games / wall_time
    steps_per_sec = total_steps / wall_time
    avg_steps_per_game = total_steps / games

    print(f"\n## Configuration")
    print(f"- Games: {games}")
    print(f"- Seed: {seed}")
    print(f"- Threads: 1")
    print(f"- skip_trivial: True")
    print(f"- enable_profiler: True")
    print(f"- Deck: Grey Ogre mirror (Mountain x20, Grey Ogre x20)")
    print(f"- Policy: random")

    print(f"\n## Throughput")
    print(f"- **Games/sec**: {games_per_sec:.2f}")
    print(f"- **Steps/sec**: {steps_per_sec:.2f}")
    print(f"- **Total steps**: {total_steps}")
    print(f"- **Wall time**: {wall_time:.3f}s")
    print(f"- **Avg steps/game**: {avg_steps_per_game:.1f}")

    # Extract profiler stats
    profiler_data = {}
    if "profiler" in final_info:
        profiler_data = parse_profiler_stats(final_info["profiler"])

        # Only show env_step hierarchy (not env_reset)
        env_step_paths = {k: v for k, v in profiler_data.items() if k.startswith("env_step")}

        env_step_time = env_step_paths.get("env_step", {}).get("total_time", 1.0)
        env_step_count = env_step_paths.get("env_step", {}).get("count", 1)

        print(f"\n## Time Breakdown")
        print(f"\n| Component | Time | % of Total | Calls | Per-call |")
        print(f"|-----------|------|------------|-------|----------|")

        # Sort by path for consistent output
        sorted_paths = sorted(env_step_paths.keys())
        for path in sorted_paths:
            stats = env_step_paths[path]
            total_time = stats.get("total_time", 0)
            count = stats.get("count", 0)
            pct = (total_time / env_step_time * 100) if env_step_time > 0 else 0
            per_call = (total_time / count * 1e6) if count > 0 else 0  # microseconds
            print(f"| {path} | {total_time:.3f}s | {pct:.1f}% | {count} | {per_call:.1f}us |")

    # Raw output
    print(f"\n## Raw Profiler Output")
    print("```")
    if "profiler" in final_info:
        env_step_items = {k: v for k, v in final_info["profiler"].items() if k.startswith("env_step")}
        for path in sorted(env_step_items.keys()):
            print(f"{path}: {env_step_items[path]}")
    print("```")

    # Behavior stats if available
    if "behavior" in final_info and "hero" in final_info["behavior"]:
        print(f"\n## Behavior Stats (Hero)")
        behavior = final_info["behavior"]["hero"]
        print(f"\n| Metric | Value |")
        print(f"|--------|-------|")
        for key, value in sorted(behavior.items()):
            print(f"| {key} | {value} |")

    return {
        "games_per_sec": games_per_sec,
        "steps_per_sec": steps_per_sec,
        "total_steps": total_steps,
        "wall_time": wall_time,
        "avg_steps_per_game": avg_steps_per_game,
        "profiler": profiler_data,
    }


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Profile managym throughput")
    parser.add_argument("--games", type=int, default=500, help="Number of games to run")
    parser.add_argument("--seed", type=int, default=42, help="Random seed")
    args = parser.parse_args()

    run_profile(args.games, args.seed)
