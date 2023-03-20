#include "extensions.h"
#include "options.h"

//////////////////////////////////////////////////////////////////////
// For sorting colors

int color_features_compare(const void* vptr_a, const void* vptr_b) {

	const color_features_t* a = (const color_features_t*)vptr_a;
	const color_features_t* b = (const color_features_t*)vptr_b;

	int u = cmp(a->user_index, b->user_index);
	if (u) { return u; }

	int w = cmp(a->wall_dist[0], b->wall_dist[0]);
	if (w) { return w; }

	int g = -cmp(a->wall_dist[1], b->wall_dist[1]);
	if (g) { return g; }

	return -cmp(a->min_dist, b->min_dist);

}

//////////////////////////////////////////////////////////////////////
// Place the game colors into a set order

void game_order_colors(game_info_t* info,
                       game_state_t* state) {

	if (g_options.order_random) {
    
		srand(now() * 1e6);
    
		for (size_t i=info->num_colors-1; i>0; --i) {
			size_t j = rand() % (i+1);
			int tmp = info->color_order[i];
			info->color_order[i] = info->color_order[j];
			info->color_order[j] = tmp;
		}

	} else { // not random

		color_features_t cf[MAX_COLORS];
		memset(cf, 0, sizeof(cf));

		for (size_t color=0; color<info->num_colors; ++color) {
			cf[color].index = color;
			cf[color].user_index = MAX_COLORS;
		}
    

		for (size_t color=0; color<info->num_colors; ++color) {
			
			int x[2], y[2];
			
			for (int i=0; i<2; ++i) {
				pos_get_coords(state->pos[color], x+i, y+i);
				cf[color].wall_dist[i] = get_wall_dist(info, x[i], y[i]);
			}

			int dx = abs(x[1]-x[0]);
			int dy = abs(y[1]-y[0]);
			
			cf[color].min_dist = dx + dy;
			
		

		}


		qsort(cf, info->num_colors, sizeof(color_features_t),
		      color_features_compare);

		for (size_t i=0; i<info->num_colors; ++i) {
			info->color_order[i] = cf[i].index;
		}
    
	}

	if (!g_options.display_quiet) {

		printf("\n************************************************"
		       "\n*               Branching Order                *\n");
		if (g_options.order_most_constrained) {
			printf("* Will choose color by most constrained\n");
		} else {
			printf("* Will choose colors in order: ");
			for (size_t i=0; i<info->num_colors; ++i) {
				int color = info->color_order[i];
				printf("%s", color_name_str(info, color));
			}
			printf("\n");
		}
		printf ("*************************************************\n\n");

	}

}



//////////////////////////////////////////////////////////////////////
// Check for dead-end regions of freespace where there is no way to
// put an active path into and out of it. Any freespace node which
// has only one free neighbor represents such a dead end. For the
// purposes of this check, cur and goal positions count as "free".

int game_check_deadends(const game_info_t* info,
                        const game_state_t* state) {
	/**
	 * FILL CODE TO DETECT DEAD-ENDS: search all free cells to see if it is a dead-end, return 1 is there is
	 */
	int x, y, dir, type;
	size_t colour = state->last_color;
	if (colour >= info->num_colors) {
		return 0;
	}

	pos_t head = state->pos[colour];
	pos_get_coords(head, &x, &y);

	// iterate through surronding 4 cells
	for (dir=0; dir<4; ++dir) {
		pos_t neighbor_pos = offset_pos(info, x, y, dir);
		type = cell_get_type(state->cells[neighbor_pos]);

		// check that cell is free and valid
		if (neighbor_pos != INVALID_POS && type == TYPE_FREE){
			int newx, newy, dir_nb;
			pos_get_coords(neighbor_pos, &newx, &newy);
			int num_free=0;

			// search neighbouring cells
			for (dir_nb=0; dir_nb<4; ++dir_nb) {
				pos_t new_neighbor_pos = offset_pos(info, newx, newy, dir_nb);
				type = cell_get_type(state->cells[new_neighbor_pos]);

				// check neighbouring cell is valid and free
				if (new_neighbor_pos != INVALID_POS && type == TYPE_FREE){
					int next_dir, nextx, nexty;
					pos_get_coords(new_neighbor_pos, &nextx, &nexty);
					int next_free=0;

					// search neighbouring cells of the surrounding cell
					for (next_dir=0; next_dir<4; ++next_dir) {
						pos_t adj_pos = offset_pos(info, nextx, nexty, next_dir);
						// check neighbouring cell is valid
						if (adj_pos != INVALID_POS){
							type = cell_get_type(state->cells[adj_pos]);
							// count free cells
							if (type == TYPE_FREE) {
								++next_free;
							}
							else {
								for (size_t color=0; color<info->num_colors; ++color) {
									// iterate to new cell if current cell is a completed path
									if (state->completed & (1 << color)) {
										continue;
									}
									// count in-progress paths and goal positions
									if (adj_pos == state->pos[color] || adj_pos == info->goal_pos[color]) {
										++next_free;
									}
								}
							}
						}
					}
					if (next_free <= 1) {
						return 1;
					}
				}

				// check neighbouring cell is valid
				if (new_neighbor_pos != INVALID_POS) {
					type = cell_get_type(state->cells[new_neighbor_pos]);
					// count free cells
					if (type == TYPE_FREE) {
						++num_free;
					}
					else {
						for (size_t color=0; color<info->num_colors; ++color) {
							// iterate to new cell if current cell is a completed path
							if (state->completed & (1 << color)) {
								continue;
							}
							// count in-progress paths and goal positions
							if (new_neighbor_pos == state->pos[color] || new_neighbor_pos == info->goal_pos[color]) {
								++num_free;
							}
						}
					}
				}

			}
			if (num_free <= 1) {
				return 1;
			}
		}
		
		
	}	

	return 0;
}
                                         
