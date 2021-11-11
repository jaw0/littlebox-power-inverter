
// printf "%d, ", 16384 * sin($_ * 3.14159265359 / 210) for(0 .. 420)
// we include an extra point, to simplify logic

short sin_data[] = {
0, 245, 490, 735, 980, 1224, 1469, 1713, 1956, 2199, 2442, 2684, 2925, 3166, 3406, 3646, 3884, 4122, 4359, 4595, 4829, 5063, 5295, 5527, 5757, 5986, 6213, 6439, 6664, 6887, 7109, 7329, 7547, 7764, 7979, 8192, 8403, 8613, 8820, 9026, 9229, 9431, 9630, 9827, 10022, 10215, 10406, 10594, 10780, 10963, 11144, 11322, 11498, 11672, 11842, 12010, 12176, 12338, 12498, 12655, 12810, 12961, 13109, 13255, 13398, 13537, 13674, 13807, 13938, 14065, 14189, 14310, 14428, 14542, 14653, 14761, 14866, 14968, 15066, 15160, 15251, 15339, 15424, 15505, 15582, 15656, 15727, 15794, 15857, 15917, 15973, 16026, 16075, 16121, 16163, 16201, 16236, 16267, 16294, 16318, 16338, 16355, 16368, 16377, 16382, 16384, 16382, 16377, 16368, 16355, 16338, 16318, 16294, 16267, 16236, 16201, 16163, 16121, 16075, 16026, 15973, 15917, 15857, 15794, 15727, 15656, 15582, 15505, 15424, 15339, 15251, 15160, 15066, 14968, 14866, 14761, 14653, 14542, 14428, 14310, 14189, 14065, 13938, 13807, 13674, 13537, 13398, 13255, 13109, 12961, 12810, 12655, 12498, 12338, 12176, 12010, 11842, 11672, 11498, 11322, 11144, 10963, 10780, 10594, 10406, 10215, 10022, 9827, 9630, 9431, 9229, 9026, 8820, 8613, 8403, 8192, 7979, 7764, 7547, 7329, 7109, 6887, 6664, 6439, 6213, 5986, 5757, 5527, 5295, 5063, 4829, 4595, 4359, 4122, 3884, 3646, 3406, 3166, 2925, 2684, 2442, 2199, 1956, 1713, 1469, 1224, 980, 735, 490, 245, 0, -245, -490, -735, -980, -1224, -1469, -1713, -1956, -2199, -2442, -2684, -2925, -3166, -3406, -3646, -3884, -4122, -4359, -4595, -4829, -5063, -5295, -5527, -5757, -5986, -6213, -6439, -6664, -6887, -7109, -7329, -7547, -7764, -7979, -8192, -8403, -8613, -8820, -9026, -9229, -9431, -9630, -9827, -10022, -10215, -10406, -10594, -10780, -10963, -11144, -11322, -11498, -11672, -11842, -12010, -12176, -12338, -12498, -12655, -12810, -12961, -13109, -13255, -13398, -13537, -13674, -13807, -13938, -14065, -14189, -14310, -14428, -14542, -14653, -14761, -14866, -14968, -15066, -15160, -15251, -15339, -15424, -15505, -15582, -15656, -15727, -15794, -15857, -15917, -15973, -16026, -16075, -16121, -16163, -16201, -16236, -16267, -16294, -16318, -16338, -16355, -16368, -16377, -16382, -16384, -16382, -16377, -16368, -16355, -16338, -16318, -16294, -16267, -16236, -16201, -16163, -16121, -16075, -16026, -15973, -15917, -15857, -15794, -15727, -15656, -15582, -15505, -15424, -15339, -15251, -15160, -15066, -14968, -14866, -14761, -14653, -14542, -14428, -14310, -14189, -14065, -13938, -13807, -13674, -13537, -13398, -13255, -13109, -12961, -12810, -12655, -12498, -12338, -12176, -12010, -11842, -11672, -11498, -11322, -11144, -10963, -10780, -10594, -10406, -10215, -10022, -9827, -9630, -9431, -9229, -9026, -8820, -8613, -8403, -8192, -7979, -7764, -7547, -7329, -7109, -6887, -6664, -6439, -6213, -5986, -5757, -5527, -5295, -5063, -4829, -4595, -4359, -4122, -3884, -3646, -3406, -3166, -2925, -2684, -2442, -2199, -1956, -1713, -1469, -1224, -980, -735, -490, -245, 0,
};