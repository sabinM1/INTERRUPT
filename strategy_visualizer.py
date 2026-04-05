import matplotlib.pyplot as plt
import numpy as np
import math
import argparse
import sys
from typing import List, Tuple

# Configuration
ARENA_RADIUS = 20.0  # Arena radius in coordinate units
START_Y_OFFSET = 5.0  # Starting position offset from center
FIGURE_SIZE = (16, 10)  # Figure size in inches
STRATEGY_NAMES = [
    "AGGRESSIVE_SCAN",
    "HARD_PIVOT_CHARGE", 
    "POWER_SLIDE_ARC",
    "DRAMATIC_BURST",
    "AGGRESSIVE_BAIT",
    "POWER_RAMP_CHARGE",
    "WIDE_SWEEP_BURST",
    "EXPERIMENTAL_SLIDE"
]
STRATEGY_DURATIONS = [
    "1200ms: Oscillation -> Power alignment",
    "900ms: Hard pivot -> Maximum charge -> Alignment",
    "1100ms: Power slide arc -> Straightening -> Alignment",
    "1100ms: Tense hold -> Turns -> Charge -> Alignment",
    "900ms: Reverse bait -> Forward slide -> Straightening -> Alignment",
    "1000ms: Exponential ramp -> Hard correction -> Power charge -> Alignment",
    "800ms: Sweeps -> Weaving burst -> Straight charge",
    "1000ms: Power slide -> Recovery turn -> Maximum charge -> Alignment"
]

def clamp_to_arena(x: float, y: float, max_radius: float = ARENA_RADIUS) -> Tuple[float, float]:
    """Clamp coordinates to stay within circular arena boundary."""
    distance = math.sqrt(x*x + y*y)
    if distance > max_radius:
        scale = max_radius / distance
        return x * scale, y * scale
    return x, y

def create_arena_background(ax, radius: float = ARENA_RADIUS):
    """Create circular arena background on matplotlib axis."""
    # Draw circular boundary
    theta = np.linspace(0, 2 * np.pi, 200)
    x_circle = radius * np.cos(theta)
    y_circle = radius * np.sin(theta)
    ax.plot(x_circle, y_circle, 'k-', linewidth=3, alpha=0.7, label='Arena Boundary')
    
    # Add center marker
    ax.plot(0, 0, 'k+', markersize=10, alpha=0.5)
    
    # Set equal aspect ratio and limits
    ax.set_aspect('equal', 'box')
    ax.set_xlim(-radius * 1.2, radius * 1.2)
    ax.set_ylim(-radius * 1.2, radius * 1.2)
    
    # Add grid
    ax.grid(True, alpha=0.3, linestyle='--')
    ax.axhline(y=0, color='gray', alpha=0.2)
    ax.axvline(x=0, color='gray', alpha=0.2)

def strategy_0_aggressive_scan() -> Tuple[List[float], List[float]]:
    """Strategy 0: AGGRESSIVE_SCAN - Dramatic oscillation scan"""
    x_points, y_points = [0], [START_Y_OFFSET]
    x, y = 0, START_Y_OFFSET
    x, y = clamp_to_arena(x, y)  # Ensure start is within bounds
    
    # Phase 1: Oscillation scan (1000ms) - 5 cycles of 200ms
    for cycle in range(5):
        # Hard left turn (50ms)
        for _ in range(3):
            x -= 1.5
            y += 0.5
            x, y = clamp_to_arena(x, y)
            x_points.append(x)
            y_points.append(y)
        
        # Hard right turn (50ms)
        for _ in range(3):
            x += 1.5
            y += 0.5
            x, y = clamp_to_arena(x, y)
            x_points.append(x)
            y_points.append(y)
        
        # Forward burst (50ms)
        for _ in range(2):
            y += 1.5
            x, y = clamp_to_arena(x, y)
            x_points.append(x)
            y_points.append(y)
        
        # Wide oscillation (50ms)
        for _ in range(2):
            x -= 0.8
            y += 0.5
            x, y = clamp_to_arena(x, y)
            x_points.append(x)
            y_points.append(y)
        for _ in range(2):
            x += 0.8
            y += 0.5
            x, y = clamp_to_arena(x, y)
            x_points.append(x)
            y_points.append(y)
    
    # Phase 2: Power forward alignment (200ms)
    for _ in range(8):
        y += 1
        x, y = clamp_to_arena(x, y)
        x_points.append(x)
        y_points.append(y)
    
    return x_points, y_points

def strategy_1_hard_pivot_charge() -> Tuple[List[float], List[float]]:
    """Strategy 1: HARD_PIVOT_CHARGE - Hard pivot with maximum charge"""
    x_points, y_points = [0], [START_Y_OFFSET]
    x, y = 0, START_Y_OFFSET
    x, y = clamp_to_arena(x, y)  # Ensure start is within bounds
    
    # Phase 1: Hard pivot rotation (500ms) - 180° pivot
    radius = 8
    for angle in range(0, 181, 10):
        rad = math.radians(angle)
        x = radius * math.cos(rad)
        y = START_Y_OFFSET + radius * math.sin(rad)
        x, y = clamp_to_arena(x, y)
        x_points.append(x)
        y_points.append(y)
    
    # Get last point
    x, y = x_points[-1], y_points[-1]
    
    # Phase 2: Maximum power charge (300ms)
    for _ in range(12):
        y += 1.2
        x, y = clamp_to_arena(x, y)
        x_points.append(x)
        y_points.append(y)
    
    # Phase 3: Aggressive alignment (100ms)
    for i in range(4):
        x += 0.3 if i % 2 == 0 else -0.3
        y += 0.5
        x, y = clamp_to_arena(x, y)
        x_points.append(x)
        y_points.append(y)
    
    return x_points, y_points

def strategy_2_power_slide_arc() -> Tuple[List[float], List[float]]:
    """Strategy 2: POWER_SLIDE_ARC - Wide power slide arc"""
    x_points, y_points = [0], [START_Y_OFFSET]
    x, y = 0, START_Y_OFFSET
    x, y = clamp_to_arena(x, y)  # Ensure start is within bounds
    
    # Phase 1: Wide arc with power slide (800ms) - 90° arc
    radius = 10
    for angle in range(0, 91, 6):
        rad = math.radians(angle)
        x = radius * math.cos(rad)
        y = START_Y_OFFSET + radius * math.sin(rad)
        x, y = clamp_to_arena(x, y)
        x_points.append(x)
        y_points.append(y)
    
    # Get last point
    x, y = x_points[-1], y_points[-1]
    
    # Phase 2: Aggressive straightening (250ms)
    for _ in range(10):
        x -= 0.6
        y += 0.8
        x, y = clamp_to_arena(x, y)
        x_points.append(x)
        y_points.append(y)
    
    # Phase 3: Final power alignment (50ms)
    for _ in range(2):
        y += 1
        x, y = clamp_to_arena(x, y)
        x_points.append(x)
        y_points.append(y)
    
    return x_points, y_points

def strategy_3_dramatic_burst() -> Tuple[List[float], List[float]]:
    """Strategy 3: DRAMATIC_BURST - Tense hold with hard turns"""
    x_points, y_points = [0], [START_Y_OFFSET]
    x, y = 0, START_Y_OFFSET
    x, y = clamp_to_arena(x, y)  # Ensure start is within bounds
    
    # Phase 1: Tense hold with vibration (400ms)
    for i in range(8):
        x += 0.5 if i % 2 == 0 else -0.5
        x, y = clamp_to_arena(x, y)
        x_points.append(x)
        y_points.append(y)
    
    # Phase 2: Dramatic left hard turn (200ms)
    for _ in range(6):
        x -= 1.2
        y += 0.7
        x, y = clamp_to_arena(x, y)
        x_points.append(x)
        y_points.append(y)
    
    # Phase 3: Dramatic right hard turn (200ms)
    for _ in range(6):
        x += 1.2
        y += 0.7
        x, y = clamp_to_arena(x, y)
        x_points.append(x)
        y_points.append(y)
    
    # Phase 4: Maximum power charge (400ms)
    for _ in range(12):
        y += 1
        x, y = clamp_to_arena(x, y)
        x_points.append(x)
        y_points.append(y)
    
    # Phase 5: Aggressive alignment (100ms)
    for i in range(3):
        x += 0.4 if i % 2 == 0 else -0.4
        y += 0.7
        x, y = clamp_to_arena(x, y)
        x_points.append(x)
        y_points.append(y)
    
    return x_points, y_points

def strategy_4_aggressive_bait() -> Tuple[List[float], List[float]]:
    """Strategy 4: AGGRESSIVE_BAIT - Reverse power slide bait"""
    x_points, y_points = [0], [START_Y_OFFSET]
    x, y = 0, START_Y_OFFSET
    x, y = clamp_to_arena(x, y)  # Ensure start is within bounds
    
    # Phase 1: Aggressive reverse with hard turn (300ms)
    for _ in range(8):
        x -= 0.7
        y -= 0.7
        x, y = clamp_to_arena(x, y)
        x_points.append(x)
        y_points.append(y)
    
    # Phase 2: Dramatic forward power slide (400ms)
    for _ in range(10):
        x += 0.8
        y += 0.8
        x, y = clamp_to_arena(x, y)
        x_points.append(x)
        y_points.append(y)
    
    # Phase 3: Power straightening with oscillation (150ms)
    for i in range(6):
        x += 0.5 if i % 2 == 0 else -0.5
        y += 0.5
        x, y = clamp_to_arena(x, y)
        x_points.append(x)
        y_points.append(y)
    
    # Phase 4: Final aggressive alignment (50ms)
    for _ in range(2):
        y += 0.8
        x, y = clamp_to_arena(x, y)
        x_points.append(x)
        y_points.append(y)
    
    return x_points, y_points

def strategy_5_power_ramp_charge() -> Tuple[List[float], List[float]]:
    """Strategy 5: POWER_RAMP_CHARGE - Exponential power ramp"""
    x_points, y_points = [0], [START_Y_OFFSET]
    x, y = 0, START_Y_OFFSET
    x, y = clamp_to_arena(x, y)  # Ensure start is within bounds
    
    # Phase 1: Exponential ramp with weaving (500ms)
    for i in range(15):
        speed = 0.5 + (i * i) / 225  # Exponential curve
        x += 0.3 if i % 2 == 0 else -0.3
        y += speed
        x, y = clamp_to_arena(x, y)
        x_points.append(x)
        y_points.append(y)
    
    # Phase 2: Aggressive correction with hard pivot (100ms)
    for _ in range(3):
        x -= 1
        x, y = clamp_to_arena(x, y)
        x_points.append(x)
        y_points.append(y)
    
    # Phase 3: Maximum power charge with weaving (300ms)
    for i in range(8):
        x += 0.6 if i % 2 == 0 else -0.6
        y += 1
        x, y = clamp_to_arena(x, y)
        x_points.append(x)
        y_points.append(y)
    
    # Phase 4: Aggressive alignment (100ms)
    for i in range(3):
        x += 0.4 if i % 2 == 0 else -0.4
        y += 0.7
        x, y = clamp_to_arena(x, y)
        x_points.append(x)
        y_points.append(y)
    
    return x_points, y_points

def strategy_6_wide_sweep_burst() -> Tuple[List[float], List[float]]:
    """Strategy 6: WIDE_SWEEP_BURST - Wide aggressive sweeps"""
    x_points, y_points = [0], [START_Y_OFFSET]
    x, y = 0, START_Y_OFFSET
    x, y = clamp_to_arena(x, y)  # Ensure start is within bounds
    
    # Phase 1: Wide aggressive left sweep (200ms)
    for _ in range(6):
        x -= 1.5
        y += 0.5
        x, y = clamp_to_arena(x, y)
        x_points.append(x)
        y_points.append(y)
    
    # Phase 2: Wide aggressive right sweep (200ms)
    for _ in range(6):
        x += 1.5
        y += 0.5
        x, y = clamp_to_arena(x, y)
        x_points.append(x)
        y_points.append(y)
    
    # Phase 3: Power burst with weaving (200ms)
    for i in range(6):
        x += 0.8 if i % 2 == 0 else -0.8
        y += 0.8
        x, y = clamp_to_arena(x, y)
        x_points.append(x)
        y_points.append(y)
    
    # Phase 4: Maximum straight charge (200ms)
    for _ in range(6):
        y += 1
        x, y = clamp_to_arena(x, y)
        x_points.append(x)
        y_points.append(y)
    
    # Phase 5: Aggressive alignment (200ms)
    for i in range(6):
        x += 0.6 if i % 3 == 0 else -0.3
        y += 0.5
        x, y = clamp_to_arena(x, y)
        x_points.append(x)
        y_points.append(y)
    
    return x_points, y_points

def strategy_7_experimental_slide() -> Tuple[List[float], List[float]]:
    """Strategy 7: EXPERIMENTAL_SLIDE - Experimental power slide"""
    x_points, y_points = [0], [START_Y_OFFSET]
    x, y = 0, START_Y_OFFSET
    x, y = clamp_to_arena(x, y)  # Ensure start is within bounds
    
    # Phase 1: Dramatic power slide with hard turn (400ms)
    for _ in range(10):
        x += 1
        x, y = clamp_to_arena(x, y)
        x_points.append(x)
        y_points.append(y)
    
    # Phase 2: Aggressive recovery with opposite turn (200ms)
    for _ in range(5):
        x -= 1
        y += 0.8
        x, y = clamp_to_arena(x, y)
        x_points.append(x)
        y_points.append(y)
    
    # Phase 3: Maximum power forward charge (300ms)
    for _ in range(8):
        y += 1.2
        x, y = clamp_to_arena(x, y)
        x_points.append(x)
        y_points.append(y)
    
    # Phase 4: Power alignment with aggressive oscillation (100ms)
    for i in range(3):
        x += 0.6 if i % 2 == 0 else -0.6
        y += 0.7
        x, y = clamp_to_arena(x, y)
        x_points.append(x)
        y_points.append(y)
    
    return x_points, y_points

def generate_ascii_visualization(strategy_idx: int, grid_size: int = 50, use_unicode: bool = True) -> str:
    """Generate high-quality ASCII visualization for a strategy."""
    # Get strategy function
    strategy_funcs = [
        strategy_0_aggressive_scan,
        strategy_1_hard_pivot_charge,
        strategy_2_power_slide_arc,
        strategy_3_dramatic_burst,
        strategy_4_aggressive_bait,
        strategy_5_power_ramp_charge,
        strategy_6_wide_sweep_burst,
        strategy_7_experimental_slide
    ]
    
    if strategy_idx < 0 or strategy_idx >= len(strategy_funcs):
        return f"Error: Strategy index {strategy_idx} is out of range (0-7)"
    
    # Get path points
    x_points, y_points = strategy_funcs[strategy_idx]()
    
    # Create ASCII arena
    width, height = grid_size, grid_size
    arena = [[' ' for _ in range(width)] for _ in range(height)]
    
    # Define characters based on Unicode preference
    boundary_char = '█' if use_unicode else '#'
    path_char = '●' if use_unicode else '*'
    interp_char = '·' if use_unicode else '.'
    
    # Draw circular boundary
    center_x, center_y = width // 2, height // 2
    radius = grid_size // 2 - 2
    
    for y in range(height):
        for x in range(width):
            dx = x - center_x
            dy = y - center_y
            distance = math.sqrt(dx*dx + dy*dy)
            if abs(distance - radius) < 0.8:
                arena[y][x] = boundary_char
    
    # Scale coordinates to fit grid
    # Find bounding box of path
    if x_points and y_points:
        min_x, max_x = min(x_points), max(x_points)
        min_y, max_y = min(y_points), max(y_points)
        path_width = max_x - min_x
        path_height = max_y - min_y
        
        # Calculate scaling factor to fit path in 80% of arena
        scale_x = (radius * 0.8) / max(abs(path_width), 0.1) if path_width > 0 else 1
        scale_y = (radius * 0.8) / max(abs(path_height), 0.1) if path_height > 0 else 1
        scale = min(scale_x, scale_y)
    else:
        scale = 1.0
    
    # Draw path with interpolation for continuous line
    for i in range(len(x_points)):
        x, y = x_points[i], y_points[i]
        scaled_x = int(center_x + x * scale)
        scaled_y = int(center_y + y * scale)
        
        # Draw current point
        if 0 <= scaled_x < width and 0 <= scaled_y < height:
            if arena[scaled_y][scaled_x] != boundary_char:
                arena[scaled_y][scaled_x] = path_char
        
        # Interpolate between points for continuous line
        if i > 0:
            prev_x, prev_y = x_points[i-1], y_points[i-1]
            steps = max(int(math.hypot(x-prev_x, y-prev_y) * scale * 2), 1)
            for step in range(1, steps):
                interp_x = prev_x + (x - prev_x) * step / steps
                interp_y = prev_y + (y - prev_y) * step / steps
                ix = int(center_x + interp_x * scale)
                iy = int(center_y + interp_y * scale)
                if 0 <= ix < width and 0 <= iy < height:
                    if arena[iy][ix] != boundary_char:
                        arena[iy][ix] = interp_char
    
    # Mark starting position
    start_x = center_x
    start_y = center_y + int(START_Y_OFFSET * scale)
    if 0 <= start_x < width and 0 <= start_y < height:
        arena[start_y][start_x] = 'S'
    
    # Mark end position
    if x_points and y_points:
        end_x = int(center_x + x_points[-1] * scale)
        end_y = int(center_y + y_points[-1] * scale)
        if 0 <= end_x < width and 0 <= end_y < height:
            if arena[end_y][end_x] != boundary_char:
                arena[end_y][end_x] = 'E'
    
    # Convert to string
    lines = [f"Strategy {strategy_idx}: {STRATEGY_NAMES[strategy_idx]}"]
    lines.append(f"Duration: {STRATEGY_DURATIONS[strategy_idx]}")
    lines.append(f"ASCII Visualization ({grid_size}x{grid_size} arena):")
    if use_unicode:
        lines.append("Legend: '█' = Arena boundary, 'S' = Start, 'E' = End, '●' = Path points, '·' = Interpolated path")
    else:
        lines.append("Legend: '#' = Arena boundary, 'S' = Start, 'E' = End, '*' = Path points, '.' = Interpolated path")
    lines.append("")
    
    for row in arena:
        lines.append(''.join(row))
    
    return '\n'.join(lines)

def generate_ascii_markdown(output_file: str = "STRATEGY_ASCII_VISUALIZATIONS.md"):
    """Generate markdown file with ASCII visualizations of all strategies."""
    markdown_lines = [
        "# Sumo Robot AGGRESSIVE Strategy ASCII Visualizations",
        "",
        "## Arena Setup",
        f"Each visualization shows a 50x50 character circular arena with the robot starting from the center, offset downward by {START_Y_OFFSET} units.",
        "The arena boundary is represented by '█' characters.",
        "",
        "Legend:",
        "- '█' = Arena boundary",
        "- 'S' = Starting position",
        "- 'E' = Ending position", 
        "- '●' = Primary path points",
        "- '·' = Interpolated path (shows continuous movement)",
        "",
        "## Strategy Details",
        ""
    ]
    
    # Add each strategy visualization
    for i in range(8):
        markdown_lines.append(f"## Strategy {i}: {STRATEGY_NAMES[i]}")
        markdown_lines.append(f"**Duration**: {STRATEGY_DURATIONS[i]}")
        markdown_lines.append("")
        markdown_lines.append("```")
        
        ascii_art = generate_ascii_visualization(i, 50, use_unicode=True)
        # Extract just the ASCII art part (skip header lines)
        ascii_lines = ascii_art.split('\n')
        # Skip first 5 lines (header) and take the rest
        art_lines = ascii_lines[5:] if len(ascii_lines) > 5 else ascii_lines
        markdown_lines.extend(art_lines)
        
        markdown_lines.append("```")
        markdown_lines.append("")
        
        # Add strategy description
        descriptions = [
            "**AGGRESSIVE_SCAN**: Dramatic zigzag oscillation with hard turns and forward bursts.",
            "**HARD_PIVOT_CHARGE**: 180° pivot rotation followed by maximum power charge.",
            "**POWER_SLIDE_ARC**: Wide curved arc with aggressive straightening maneuver.",
            "**DRAMATIC_BURST**: Tense vibration hold leading to hard turns and maximum charge.",
            "**AGGRESSIVE_BAIT**: Reverse power slide bait followed by forward recovery.",
            "**POWER_RAMP_CHARGE**: Exponential acceleration ramp with weaving pattern.",
            "**WIDE_SWEEP_BURST**: Wide left/right sweeps transitioning to weaving burst.",
            "**EXPERIMENTAL_SLIDE**: Power slide with recovery turn and maximum forward charge."
        ]
        markdown_lines.append(descriptions[i])
        markdown_lines.append("")
    
    # Add footer
    markdown_lines.append("---")
    markdown_lines.append("*Generated by strategy_visualizer.py*")
    
    # Write to file
    content = '\n'.join(markdown_lines)
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write(content)
    
    print(f"Generated ASCII markdown file: {output_file}")
    return output_file

def create_ascii_fallback(strategy_idx: int) -> str:
    """Generate ASCII visualization as fallback when matplotlib is not available."""
    return generate_ascii_visualization(strategy_idx, 50, use_unicode=False)

def visualize_strategies_matplotlib(output_file: str = None, show_plot: bool = True):
    """Create matplotlib visualization of all 8 strategies."""
    # Create figure with 2x4 grid
    fig, axes = plt.subplots(2, 4, figsize=FIGURE_SIZE)
    fig.suptitle('Sumo Robot AGGRESSIVE Strategy Visualizations', fontsize=16, fontweight='bold')
    
    # Get all strategy functions
    strategy_funcs = [
        strategy_0_aggressive_scan,
        strategy_1_hard_pivot_charge,
        strategy_2_power_slide_arc,
        strategy_3_dramatic_burst,
        strategy_4_aggressive_bait,
        strategy_5_power_ramp_charge,
        strategy_6_wide_sweep_burst,
        strategy_7_experimental_slide
    ]
    
    for idx, (ax, strategy_func, name, duration) in enumerate(zip(
        axes.flat, strategy_funcs, STRATEGY_NAMES, STRATEGY_DURATIONS
    )):
        # Create arena background
        create_arena_background(ax)
        
        # Get strategy path
        x_points, y_points = strategy_func()
        
        # Plot strategy path with color gradient based on progress
        colors = plt.cm.viridis(np.linspace(0, 1, len(x_points)))
        for i in range(1, len(x_points)):
            ax.plot(x_points[i-1:i+1], y_points[i-1:i+1], 
                   color=colors[i], linewidth=2, alpha=0.8)
        
        # Mark start and end points
        ax.plot(x_points[0], y_points[0], 'go', markersize=8, label='Start', zorder=5)
        ax.plot(x_points[-1], y_points[-1], 'ro', markersize=8, label='End', zorder=5)
        
        # Add title and labels
        ax.set_title(f"Strategy {idx}: {name}", fontsize=10, fontweight='bold')
        ax.text(0.02, 0.98, duration.split(':')[0], 
                transform=ax.transAxes, fontsize=8, 
                verticalalignment='top', bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.8))
        
        # Add legend only to first subplot
        if idx == 0:
            ax.legend(loc='upper left', fontsize=7)
    
    # Adjust layout
    plt.tight_layout(rect=[0, 0.03, 1, 0.95])
    
    # Add overall description
    fig.text(0.02, 0.02, 
             "Visualization of 8 aggressive sumo robot opener strategies.\n"
             "Each strategy shows the robot's movement pattern starting from 'S' (green) and ending at 'E' (red).\n"
             "Paths show progression from dark (start) to light (end) colors.",
             fontsize=9, style='italic', 
             bbox=dict(boxstyle='round', facecolor='lightgray', alpha=0.3))
    
    # Save or show
    if output_file:
        plt.savefig(output_file, dpi=300, bbox_inches='tight')
        print(f"Saved visualization to {output_file}")
    
    if show_plot:
        plt.show()
    else:
        plt.close()

def visualize_single_strategy_matplotlib(strategy_idx: int, output_file: str = None, show_plot: bool = True):
    """Create detailed visualization of a single strategy."""
    if strategy_idx < 0 or strategy_idx >= len(STRATEGY_NAMES):
        print(f"Error: Strategy index must be between 0 and {len(STRATEGY_NAMES)-1}")
        return
    
    # Get strategy function
    strategy_funcs = [
        strategy_0_aggressive_scan,
        strategy_1_hard_pivot_charge,
        strategy_2_power_slide_arc,
        strategy_3_dramatic_burst,
        strategy_4_aggressive_bait,
        strategy_5_power_ramp_charge,
        strategy_6_wide_sweep_burst,
        strategy_7_experimental_slide
    ]
    
    strategy_func = strategy_funcs[strategy_idx]
    
    # Create figure
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))
    fig.suptitle(f'Strategy {strategy_idx}: {STRATEGY_NAMES[strategy_idx]}', 
                 fontsize=14, fontweight='bold')
    
    # Plot 1: Strategy path in arena
    create_arena_background(ax1)
    x_points, y_points = strategy_func()
    
    # Plot with color gradient
    colors = plt.cm.plasma(np.linspace(0, 1, len(x_points)))
    for i in range(1, len(x_points)):
        ax1.plot(x_points[i-1:i+1], y_points[i-1:i+1], 
                color=colors[i], linewidth=3, alpha=0.8)
    
    # Mark points
    ax1.plot(x_points[0], y_points[0], 'go', markersize=10, label='Start', zorder=5)
    ax1.plot(x_points[-1], y_points[-1], 'ro', markersize=10, label='End', zorder=5)
    
    # Add arrows to show direction
    arrow_indices = np.linspace(0, len(x_points)-2, 5, dtype=int)
    for idx in arrow_indices:
        if idx + 1 < len(x_points):
            dx = x_points[idx+1] - x_points[idx]
            dy = y_points[idx+1] - y_points[idx]
            ax1.arrow(x_points[idx], y_points[idx], dx*0.8, dy*0.8,
                     head_width=0.5, head_length=0.5, fc='blue', ec='blue', alpha=0.5)
    
    ax1.set_title(f'Movement Pattern', fontsize=12)
    ax1.legend()
    
    # Plot 2: Speed profile (distance between points)
    distances = []
    for i in range(1, len(x_points)):
        dist = math.sqrt((x_points[i] - x_points[i-1])**2 + (y_points[i] - y_points[i-1])**2)
        distances.append(dist)
    
    ax2.plot(range(len(distances)), distances, 'b-', linewidth=2)
    ax2.fill_between(range(len(distances)), distances, alpha=0.3)
    ax2.set_xlabel('Step Number')
    ax2.set_ylabel('Step Distance (speed indicator)')
    ax2.set_title('Speed Profile (Relative Movement per Step)', fontsize=12)
    ax2.grid(True, alpha=0.3)
    
    # Add statistics
    stats_text = f"Total Path Length: {sum(distances):.1f} units\n"
    stats_text += f"Number of Steps: {len(x_points)}\n"
    stats_text += f"Duration: {STRATEGY_DURATIONS[strategy_idx]}\n"
    stats_text += f"Final Position: ({x_points[-1]:.1f}, {y_points[-1]:.1f})"
    
    ax2.text(0.02, 0.98, stats_text, 
             transform=ax2.transAxes, fontsize=10,
             verticalalignment='top', 
             bbox=dict(boxstyle='round', facecolor='lightblue', alpha=0.5))
    
    plt.tight_layout(rect=[0, 0.03, 1, 0.95])
    
    # Save or show
    if output_file:
        plt.savefig(output_file, dpi=300, bbox_inches='tight')
        print(f"Saved single strategy visualization to {output_file}")
    
    if show_plot:
        plt.show()
    else:
        plt.close()

def main():
    """Main function with command-line argument parsing."""
    parser = argparse.ArgumentParser(
        description='Visualize sumo robot aggressive strategies',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python strategy_visualizer.py                    # Show all strategies
  python strategy_visualizer.py --single 0         # Show only strategy 0
  python strategy_visualizer.py --output strategies.png  # Save to file
  python strategy_visualizer.py --ascii 3          # ASCII output for strategy 3
  python strategy_visualizer.py --markdown         # Generate ASCII markdown file
  python strategy_visualizer.py --no-show          # Don't display, just save
        """
    )
    
    parser.add_argument('--single', type=int, choices=range(8), 
                       help='Visualize single strategy (0-7)')
    parser.add_argument('--output', type=str, 
                       help='Output file path for saving visualization')
    parser.add_argument('--no-show', action='store_true',
                       help='Do not display plot, only save if --output specified')
    parser.add_argument('--ascii', type=int, choices=range(8),
                       help='Generate ASCII output for specific strategy (0-7)')
    parser.add_argument('--markdown', action='store_true',
                       help='Generate markdown file with ASCII visualizations')
    parser.add_argument('--grid-size', type=int, default=50,
                       help='Grid size for ASCII visualizations (default: 50)')
    
    args = parser.parse_args()
    
    # Handle ASCII markdown generation
    if args.markdown:
        output_file = args.output or "STRATEGY_ASCII_VISUALIZATIONS.md"
        generate_ascii_markdown(output_file)
        return
    
    # Handle ASCII fallback mode
    if args.ascii is not None:
        print(generate_ascii_visualization(args.ascii, args.grid_size, use_unicode=False))
        return
    
    # Check matplotlib availability
    try:
        import matplotlib
    except ImportError:
        print("Matplotlib not available. Using ASCII fallback for all strategies.")
        print("=" * 60)
        for i in range(8):
            print(generate_ascii_visualization(i, args.grid_size, use_unicode=False))
            print("=" * 60)
        return
    
    # Determine visualization mode
    show_plot = not args.no_show
    
    if args.single is not None:
        # Single strategy visualization
        output_file = args.output or f"strategy_{args.single}.png"
        visualize_single_strategy_matplotlib(args.single, output_file, show_plot)
    else:
        # All strategies visualization
        output_file = args.output or "all_strategies.png"
        visualize_strategies_matplotlib(output_file, show_plot)

if __name__ == "__main__":
    main()