import matplotlib
matplotlib.use('Agg')  # Set backend before pyplot import
import matplotlib.pyplot as plt
import numpy as np
import os
import argparse
import sys
from typing import List, Tuple

# Import from the existing strategy visualizer module
try:
    from strategy_visualizer import (
        # Strategy functions
        strategy_0_aggressive_scan,
        strategy_1_hard_pivot_charge,
        strategy_2_power_slide_arc,
        strategy_3_dramatic_burst,
        strategy_4_aggressive_bait,
        strategy_5_power_ramp_charge,
        strategy_6_wide_sweep_burst,
        strategy_7_experimental_slide,
        # Visualization functions
        visualize_strategies_matplotlib,
        visualize_single_strategy_matplotlib,
        # Configuration constants
        STRATEGY_NAMES,
        STRATEGY_DURATIONS,
        ARENA_RADIUS,
        START_Y_OFFSET,
        FIGURE_SIZE
    )
    IMPORT_SUCCESS = True
except ImportError as e:
    print(f"Error importing from strategy_visualizer.py: {e}")
    print("Make sure strategy_visualizer.py is in the same directory.")
    IMPORT_SUCCESS = False
    sys.exit(1)

def get_strategy_function(strategy_idx: int):
    """Get the strategy function for the given index."""
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
    
    if 0 <= strategy_idx < len(strategy_funcs):
        return strategy_funcs[strategy_idx]
    raise ValueError(f"Strategy index must be between 0 and {len(strategy_funcs)-1}")

def generate_single_strategy_png(strategy_idx: int, output_path: str, dpi: int = 300) -> bool:
    """Generate a PNG file for a single strategy using imported visualization."""
    try:
        # Use the imported single strategy visualization function
        visualize_single_strategy_matplotlib(strategy_idx, output_path, show_plot=False)
        print(f"✓ Generated: {output_path}")
        return True
    except Exception as e:
        print(f"✗ Error generating strategy {strategy_idx}: {e}")
        return False

def generate_all_strategies_grid(output_path: str, dpi: int = 300) -> bool:
    """Generate a grid of all 8 strategies in a single PNG using imported visualization."""
    try:
        # Use the imported grid visualization function
        visualize_strategies_matplotlib(output_path, show_plot=False)
        print(f"✓ Generated grid: {output_path}")
        return True
    except Exception as e:
        print(f"✗ Error generating strategy grid: {e}")
        return False

def generate_all_individual_strategies(output_dir: str, prefix: str = "strategy", dpi: int = 300) -> int:
    """Generate individual PNGs for all 8 strategies."""
    success_count = 0
    print(f"Generating individual PNGs for all 8 strategies in '{output_dir}'...")
    
    for strategy_idx in range(8):
        output_path = os.path.join(output_dir, f"{prefix}_{strategy_idx}.png")
        if generate_single_strategy_png(strategy_idx, output_path, dpi):
            success_count += 1
    
    return success_count

def main():
    """Main function to generate strategy PNGs using imported visualization functions."""
    parser = argparse.ArgumentParser(
        description='Generate PNG visualizations for sumo robot aggressive strategies',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python generate_strategy_pngs.py                    # Generate all individual PNGs
  python generate_strategy_pngs.py --grid             # Generate grid of all strategies
  python generate_strategy_pngs.py --single 3         # Generate only strategy 3
  python generate_strategy_pngs.py --output-dir ./images  # Custom output directory
  python generate_strategy_pngs.py --dpi 600          # Higher resolution output
        """
    )
    
    parser.add_argument('--single', type=int, choices=range(8),
                       help='Generate PNG for single strategy (0-7)')
    parser.add_argument('--grid', action='store_true',
                       help='Generate grid of all strategies in one PNG')
    parser.add_argument('--all', action='store_true',
                       help='Generate individual PNGs for all strategies')
    parser.add_argument('--output-dir', type=str, default='./strategy_pngs',
                       help='Output directory for PNG files (default: ./strategy_pngs)')
    parser.add_argument('--dpi', type=int, default=300,
                       help='DPI for PNG output (default: 300)')
    parser.add_argument('--prefix', type=str, default='strategy',
                       help='Filename prefix (default: strategy)')
    
    args = parser.parse_args()
    
    # Create output directory if it doesn't exist
    os.makedirs(args.output_dir, exist_ok=True)
    
    # Determine what to generate
    generate_grid = args.grid
    generate_single = args.single is not None
    generate_all = args.all or (not generate_grid and not generate_single)
    
    success_count = 0
    
    # Generate grid of all strategies
    if generate_grid:
        grid_path = os.path.join(args.output_dir, f"{args.prefix}_grid.png")
        if generate_all_strategies_grid(grid_path, args.dpi):
            success_count += 1
    
    # Generate single strategy
    if generate_single:
        single_path = os.path.join(args.output_dir, f"{args.prefix}_{args.single}.png")
        if generate_single_strategy_png(args.single, single_path, args.dpi):
            success_count += 1
    
    # Generate all individual strategies
    if generate_all:
        count = generate_all_individual_strategies(args.output_dir, args.prefix, args.dpi)
        success_count += count
    
    # Summary
    print("\n" + "="*60)
    print(f"SUMMARY: Generated {success_count} PNG file(s) in '{args.output_dir}'")
    print("="*60)
    
    if success_count == 0:
        print("\nUsage tips:")
        print("  python generate_strategy_pngs.py --all          # Generate all individual PNGs")
        print("  python generate_strategy_pngs.py --grid         # Generate grid PNG")
        print("  python generate_strategy_pngs.py --single 3     # Generate strategy 3 only")
        print("  python generate_strategy_pngs.py --help         # Show all options")

if __name__ == "__main__":
    main()