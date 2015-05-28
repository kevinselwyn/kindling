#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <curses.h>
#include <jansson.h>
#include <math.h>
#include <jpeglib.h>
#include <curl/curl.h>

#define API_ROOT   "https://api.gotinder.com"
#define API_AUTH   API_ROOT "/auth"
#define API_RECS   API_ROOT "/user/recs"
#define API_MSGS   API_ROOT "/updates"
#define API_OAUTH  "https://www.facebook.com/dialog/oauth?client_id=464891386855067&redirect_uri=https://www.facebook.com/connect/login_success.html&scope=basic_info,email,public_profile,user_about_me,user_activities,user_birthday,user_education_history,user_friends,user_interests,user_likes,user_location,user_photos,user_relationship_details&response_type=token"
#define API_OPEN   "open \"" API_OAUTH "\""
#define RATE_LIMIT "tinder_rate_limited"
#define USER_AGENT "Tinder/3.0.4 (iPhone; iOS 7.1; Scale/2.00)"
#define DOTFILE    ".kindling"
#define FRAMERATE  40000

int width = 80, height = 24, left = 0, container = 26;
char *chars[6] = { "─", "│", "┌", "┐", "┘", "└" };
char *patterns[5] = {
	"\033[%d;%dH%s",
	"\033[%d;%dH%*s",
	"\033[%d;%dH%s, %d",
	"\033[%d;%dH%d %s %s",
	"\033[%d;%dH%4d %s"
};

struct rec {
	int age, gender, distance, likes, friends;
	char *name, *id, *photo, *bio;
};

struct curl_response {
	char *memory;
	size_t size;
};

static size_t get_write(void *ptr, size_t size, size_t nmemb, char *res) {
	size_t real_size = size * nmemb;
	struct curl_response *mem = (struct curl_response *)res;

	mem->memory = realloc(mem->memory, mem->size + real_size + 1);

	if (mem->memory == NULL) {
		printf("Memory error\n");

		return 0;
	}

	memcpy(&(mem->memory[mem->size]), ptr, real_size);
	mem->size += real_size;
	mem->memory[mem->size] = (char)0;

	return real_size;
}

static void get_dims(int *wth, int *hth, int *lft) {
	struct winsize w;

	ioctl(0, TIOCGWINSZ, &w);

	*wth = w.ws_col;
	*hth = w.ws_row;
	*lft = (*wth - container) / 2;
}

static void draw_init() {
	initscr();
	noecho();
	curs_set(0);
	nodelay(stdscr, TRUE);
	leaveok(stdscr, TRUE);
	scrollok(stdscr, FALSE);
}

static void draw_loop() {
	doupdate();
	usleep(FRAMERATE);
}

static void draw_end() {
	endwin();
}

static void draw_clear() {
	int x = 0, y = 0;

	x = 2;

	for (y = 2; y <= height - 1; y++) {
		printf(patterns[1], y, x, width - 2, " ");
	}
}

static void draw_border() {
	int x = 0, y = 0;

	/* Border Top */
	for (x = 2, y = 1; x < width; x++) {
		if (x >= left + 1 && x <= left + 10) {
			continue;
		}

		printf(patterns[0], y, x, chars[0]);
	}

	/* Border Bottom */
	for (x = 2, y = height; x < width; x++) {
		printf(patterns[0], y, x, chars[0]);
	}

	/* Border Left */
	for (x = 1, y = 2; y < height; y++) {
		printf(patterns[0], y, x, chars[1]);
	}

	/* Border Right */
	for (x = width, y = 2; y < height; y++) {
		printf(patterns[0], y, x, chars[1]);
	}

	/* Border Corners */
	printf(patterns[0], 1, 1, chars[2]);
	printf(patterns[0], 1, width, chars[3]);
	printf(patterns[0], height, width, chars[4]);
	printf(patterns[0], height, 1, chars[5]);

	/* Border Label */
	printf(patterns[0], 1, left + 1, " kindling ");
}

static void draw_pass_border() {
	int x = 0, y = 0;

	/* Pass Top Horizontal */
	for (x = left + 1, y = 17; x < left + 9; x++) {
		printf(patterns[0], y, x, chars[0]);
	}

	/* Pass Bottom Horizontal */
	for (x = left + 1, y = 22; x < left + 9; x++) {
		printf(patterns[0], y, x, chars[0]);
	}

	/* Pass Left Vertical */
	for (x = left, y = 18; y < 22; y++) {
		printf(patterns[0], y, x, chars[1]);
	}

	/* Pass Right Vertical */
	for (x = left + 9, y = 18; y < 22; y++) {
		printf(patterns[0], y, x, chars[1]);
	}

	/* Pass Corners */
	printf(patterns[0], 17, left, chars[2]);
	printf(patterns[0], 17, left + 9, chars[3]);
	printf(patterns[0], 22, left + 9, chars[4]);
	printf(patterns[0], 22, left, chars[5]);
}

static void draw_pass_icon() {
	int i = 0, l = 0, x = 0, y = 0;
	char *icon[4] = {
		"  \\  /  ",
		"   \\/   ",
		"   /\\   ",
		"  /  \\  "
	};

	x = left + 1;
	y = 18;

	for (i = 0, l = 4; i < l; i++) {
		printf(patterns[0], y++, x, icon[i]);
	}
}

static void draw_like_border() {
	int x = 0, y = 0;

	/* Like Top Horizontal */
	for (x = left + 18, y = 17; x < left + 26; x++) {
		printf(patterns[0], y, x, chars[0]);
	}

	/* Like Bottom Horizontal */
	for (x = left + 18, y = 22; x < left + 26; x++) {
		printf(patterns[0], y, x, chars[0]);
	}

	/* Like Left Vertical */
	for (x = left + 17, y = 18; y < 22; y++) {
		printf(patterns[0], y, x, chars[1]);
	}

	/* Like Right Vertical */
	for (x = left + 26, y = 18; y < 22; y++) {
		printf(patterns[0], y, x, chars[1]);
	}

	/* Like Corners */
	printf(patterns[0], 17, left + 17, chars[2]);
	printf(patterns[0], 17, left + 26, chars[3]);
	printf(patterns[0], 22, left + 26, chars[4]);
	printf(patterns[0], 22, left + 17, chars[5]);
}

static void draw_like_icon() {
	int i = 0, l = 0, x = 0, y = 0;
	char *icon[4] = {
		" / \\/ \\ ",
		" \\    / ",
		"  \\  /  ",
		"   \\/   "
	};

	x = left + 18;
	y = 18;

	for (i = 0, l = 4; i < l; i++) {
		printf(patterns[0], y++, x, icon[i]);
	}
}

static void draw_recs() {
	int x = 0, y = 0;

	/* Rec Top Horizontal */
	for (x = left + 1, y = 3; x < left + 26; x++) {
		printf(patterns[0], y, x, chars[0]);
	}

	/* Rec Bottom Horizontal */
	for (x = left + 1, y = 15; x < left + 26; x++) {
		printf(patterns[0], y, x, chars[0]);
	}

	/* Rec Left Vertical */
	for (x = left, y = 4; y < 15; y++) {
		printf(patterns[0], y, x, chars[1]);
	}

	/* Rec Right Vertical */
	for (x = left + 26, y = 4; y < 15; y++) {
		printf(patterns[0], y, x, chars[1]);
	}

	/* Rec Corners */
	printf(patterns[0], 3, left, chars[2]);
	printf(patterns[0], 3, left + 26, chars[3]);
	printf(patterns[0], 15, left + 26, chars[4]);
	printf(patterns[0], 15, left, chars[5]);

	/* Pass Icon */
	draw_pass_border();
	draw_pass_icon();

	/* Like Icon */
	draw_like_border();
	draw_like_icon();
}

static void draw_pass() {
	/* Text Color On */
	printf("\x1B[31m");

	/* Pass Border */
	draw_pass_border();

	/* Text Color On */
	printf("\x1B[37m");

	/* Background Color On */
	printf("\x1B[41m");

	/* Pass Icon */
	draw_pass_icon();

	/* Color Off */
	printf("\x1B[0m");
}

static void draw_like() {
	/* Text Color On */
	printf("\x1B[32m");

	/* Like Border */
	draw_like_border();

	/* Text Color On */
	printf("\x1B[37m");

	/* Border Color On */
	printf("\x1B[42m");

	/* Like Icon */
	draw_like_icon();

	/* Color Off */
	printf("\x1B[0m");
}

static void draw_info(char *name, int age) {
	int x = 0, y = 0;

	x = left + 1;
	y = 16;

	printf(patterns[1], y, x, 26, " ");
	printf(patterns[2], y, x, name, age);
}

static void draw_match() {
	int x = 0, y = 0;

	x = left + 21;
	y = 2;

	printf(patterns[0], y, x, "Match!");
}

static void clear_match() {
	int x = 0, y = 0;

	x = left + 21;
	y = 2;

	printf(patterns[1], y, x, 6, " ");
}

static void draw_photo(char *photo) {
	int width = 0, i = 0, l = 0, x = 0, y = 0, lines = 0;
	size_t length = strlen(photo);
	char *line = NULL;

	x = left + 3;
	y = 4;

	for (i = 0, l = (int)length; i < l; i++) {
		if (photo[i] == '\n') {
			width = i;
			break;
		}
	}

	line = malloc(sizeof(char) * width + 1);
	lines = (int)length / width;

	for (i = 0, l = lines; i < l; i++) {
		strncpy(line, photo + ((width + 1) * i), width);
		line[width] = '\0';
		printf(patterns[0], y++, x, line);
	}

	if (line) {
		free(line);
	}
}

static void draw_bio(struct rec rec) {
	int i = 0, l = 0, x = 0, y = 0, new_width = 0, lines = 0;
	size_t length = 0;
	char *line = NULL;

	x = 3;
	y = 3;

	printf(patterns[2], y++, x, rec.name, rec.age);
	printf(patterns[3], y++, x, rec.distance, rec.distance == 1 ? "mile" : "miles", "away");
	printf(patterns[0], y, x, rec.gender == 0 ? "Male" : "Female");

	y += 2;

	new_width = width - 4;
	length = strlen(rec.bio);

	line = malloc(sizeof(char) * new_width + 1);
	lines = (int)length / new_width;

	for (i = 0, l = (int)length; i < l; i++) {
		if (rec.bio[i] == '\n') {
			rec.bio[i] = ' ';
		}
	}

	for (i = 0, l = lines; i <= l; i++) {
		strncpy(line, rec.bio + (new_width * i), new_width);
		line[new_width] = '\0';
		printf(patterns[0], y++, x, line);
	}

	x = width - 13;
	y = 3;

	printf(patterns[4], y++, x, rec.likes, "likes");
	printf(patterns[4], y, x, rec.friends, "friends");

	if (line) {
		free(line);
	}
}

static char findchar(int sample, double sample_mean, double mean, double sigma) {
	char ascii_chars[14] = { ' ', '.', '*', '@', 'O', 'o', '`', ',', '^', '[', ']', '=', '\\', '/' };

	switch (sample) {
	case 0b0000:
		if ((sample_mean >= mean + (sigma * 1)) || sample_mean >= 255) {
			return ascii_chars[0];
		} else if (sample_mean >= mean + (0.3 * sigma)) {
			return ascii_chars[1];
		} else if (sample_mean >= mean) {
			return ascii_chars[2];
		}
	case 0b1111:
		if ((sample_mean <= mean - (sigma * 1)) || sample_mean <= 0) {
			return ascii_chars[3];
		} else if (sample_mean <= mean - (0.3 * sigma)) {
			return ascii_chars[4];
		} else if (sample_mean <= mean) {
			return ascii_chars[5];
		}
		break;
	case 0b1000:
	case 0b0010:
		return ascii_chars[6];
	case 0b0100:
	case 0b0001:
		return ascii_chars[7];
		break;
	case 0b1010:
		return ascii_chars[8];
		break;
	case 0b1100:
		return ascii_chars[9];
		break;
	case 0b0011:
		return ascii_chars[10];
		break;
	case 0b0101:
		return ascii_chars[11];
		break;
	case 0b1001:
	case 0b1011:
	case 0b1101:
		return ascii_chars[12];
		break;
	case 0b0110:
	case 0b0111:
	case 0b1110:
		return ascii_chars[13];
		break;
	default:
		break;
	}

	return ascii_chars[0];
}

static void jpeg2ascii(char *url, char **ascii) {
	int i = 0, j = 0, l = 0, x = 0, y = 0, r = 0, g = 0, b = 0;
	int width = 0, height = 0;
	int row_stride = 0, count = 0, counter = 0, offset = 0, sigma = 0, sample = 0;
	double yiq = 0.0, yiq_sum = 0.0, yyy = 0.0;
	double mean = 0.0, sample_mean = 0.0;
	char *result = NULL;
	unsigned char *jpegdata = NULL, *yiqs = NULL;
	struct jpeg_decompress_struct cinfo_decompress;
	struct jpeg_error_mgr jerr;
	struct curl_response response;
	FILE *jpegfile = NULL;
	JSAMPARRAY pJpegBuffer;
	CURL *curl;
	CURLcode res;

	response.memory = malloc(1);
	response.size = 0;

	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();

	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, get_write);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);

		res = curl_easy_perform(curl);

		if (res != CURLE_OK) {
			printf("Error: %s\n", curl_easy_strerror(res));
			goto cleanup;
		}
	}

	cinfo_decompress.err = jpeg_std_error(&jerr);

	jpeg_create_decompress(&cinfo_decompress);
	jpeg_mem_src(&cinfo_decompress, (unsigned char *)response.memory, response.size);

	jpeg_read_header(&cinfo_decompress, TRUE);
	jpeg_start_decompress(&cinfo_decompress);

	width = cinfo_decompress.output_width;
	height = cinfo_decompress.output_height;
	row_stride = width * cinfo_decompress.output_components;

	pJpegBuffer = (*cinfo_decompress.mem->alloc_sarray)((j_common_ptr) &cinfo_decompress, JPOOL_IMAGE, row_stride, 1);

	jpegdata = malloc(sizeof(char) * (width * height * 3) + 1);
	yiqs = malloc(sizeof(char) * (height * width + width) + 1);

	result = malloc(sizeof(char) * (width * height) + 1);
	strcpy(result, "");

	count = 0;
	counter = 0;
	y = 0;

	while (cinfo_decompress.output_scanline < cinfo_decompress.output_height) {
		jpeg_read_scanlines(&cinfo_decompress, pJpegBuffer, 1);

		for (x = 0; x < width; x++) {
			r = pJpegBuffer[0][cinfo_decompress.output_components * x];
			g = pJpegBuffer[0][cinfo_decompress.output_components * x + 1];
			b = pJpegBuffer[0][cinfo_decompress.output_components * x + 2];

			jpegdata[counter] = r;
			jpegdata[counter + 1] = g;
			jpegdata[counter + 2] = b;

			offset = y * width + x;
			yiq = ((r * 0.299) + (g * 0.587) + (b * 0.114));
			yiqs[offset] = yiq;
			yiq_sum += yiq;

			count++;
			counter += 3;
		}

		y++;
	}

	mean = yiq_sum / count;
	sigma = 0;

	for (i = 0, l = height * width + width; i < l; i++) {
		if (yiqs[i] != '\0') {
			sigma += pow(yiqs[i] - mean, 2);
		}
	}

	sigma = sqrt(sigma / count);
	count = 0;

	for (y = 0; y < height - 2; y += 8) {
		for (x = 0; x < width - 2; x += 4) {
			sample = 0;
			sample_mean = 0.0;

			for (i = y; i < y + 2; i++) {
				for (j = x; j < x + 2; j++) {
					r = jpegdata[((i * width) + j) * 3];
					g = jpegdata[((i * width) + j) * 3 + 1];
					b = jpegdata[((i * width) + j) * 3 + 2];

					yyy = yiqs[i * width + j];

					if (yyy < mean) {
						sample |= 0b0001;
					}

					sample <<= 1;
					sample_mean += yyy;
				}
			}

			sample_mean /= 4;
			sample >>= 1;

			result[count++] = findchar(sample, sample_mean, mean, sigma);
		}

		result[count++] = '\n';
	}

	result[count] = '\0';

	*ascii = malloc(sizeof(char) * strlen(result) + 1);
	strcpy(*ascii, result);

cleanup:
	if (jpegfile) {
		fclose(jpegfile);
	}

	if (jpegdata) {
		free(jpegdata);
	}
}

static void send_request(char *method, char *url, char *header, char *query, char **data) {
	char *user_agent = NULL;
	struct curl_response response;
	struct curl_slist *headers = NULL;
	CURL *curl;
	CURLcode res;

	response.memory = malloc(1);
	response.size = 0;

	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();

	if (curl) {
		user_agent = malloc(sizeof(char) * (13 + strlen(USER_AGENT)));
		sprintf(user_agent, "User-Agent: %s", USER_AGENT);
		headers = curl_slist_append(headers, user_agent);

		if (header) {
			headers = curl_slist_append(headers, header);
		}

		if (query) {
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, query);
		}

		if (strcmp(method, "POST") == 0) {
			curl_easy_setopt(curl, CURLOPT_POST, 1L);
		}

		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, get_write);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

		res = curl_easy_perform(curl);

		if (res != CURLE_OK) {
			printf("Error: %s\n", curl_easy_strerror(res));
			goto cleanup;
		}

		*data = malloc(sizeof(char) * response.size + 1);
		strcpy(*data, response.memory);

		curl_easy_cleanup(curl);
		curl_slist_free_all(headers);
	}

cleanup:
	curl_global_cleanup();

	if (user_agent) {
		free(user_agent);
	}
}

static int get_auth(char *facebook_token, char **x_auth_token) {
	int rc = 0, code = 0;
	char *data = NULL, *query = NULL;
	const char *tmp = NULL;
	json_t *root, *code_node, *token_node;
	json_error_t error;

	query = malloc(sizeof(char) * (strlen(facebook_token) + 16));
	sprintf(query, "facebook_token=%s", facebook_token);

	send_request("POST", API_AUTH, NULL, query, &data);
	root = json_loads(data, 0, &error);

	code_node = json_object_get(root, "code");
	code = json_integer_value(code_node);

	if (code != 0) {
		printf("Please reauthorize:\n\n");

		rc = 1;
		goto cleanup;
	}

	token_node = json_object_get(root, "token");

	tmp = json_string_value(token_node);
	*x_auth_token = malloc(sizeof(char) * strlen(tmp) + 15);
	sprintf(*x_auth_token, "X-Auth-Token: %s", tmp);

cleanup:
	if (data) {
		free(data);
	}

	if (query) {
		free(query);
	}

	json_decref(root);

	return rc;
}

static size_t get_recs(char *x_auth_token, json_t **results) {
	char *data = NULL;
	size_t rec_count = 0;
	json_t *root;
	json_error_t error;

	send_request("GET", API_RECS, x_auth_token, NULL, &data);
	root = json_loads(data, 0, &error);

	*results = json_object_get(root, "results");

	rec_count = json_array_size(*results);

	if (data) {
		free(data);
	}

	return rec_count;
}

static void get_info(json_t *results, size_t index, struct rec *rec) {
	int year = 0, month = 0, day = 0, seconds = 0, i = 0, l = 0;
	char *url = NULL;
	const char *tmp = NULL;
	json_t *result, *name_node, *birth_date_node, *id_node, *gender_node;
	json_t *bio_node, *distance_node, *photos_node, *photo_node;
	json_t *processedFiles_node, *processedFile_node, *url_node, *height_node;
	json_t *likes_node, *friends_node;
	time_t start_time, end_time, today = time(NULL);
	struct tm start_date, end_date;

	result = json_array_get(results, (int)index);

	/* ID */
	id_node = json_object_get(result, "_id");

	tmp = json_string_value(id_node);
	rec->id = malloc(sizeof(char) * 25);
	strcpy(rec->id, tmp);

	/* Bio */
	bio_node = json_object_get(result, "bio");

	tmp = json_string_value(bio_node);
	rec->bio = malloc(sizeof(char) * (strlen(tmp) + 1));
	strcpy(rec->bio, tmp);

	/* Name */
	name_node = json_object_get(result, "name");

	tmp = json_string_value(name_node);
	rec->name = malloc(sizeof(char) * (strlen(tmp) + 1));
	strcpy(rec->name, tmp);

	/* Birth Date */
	birth_date_node = json_object_get(result, "birth_date");

	tmp = json_string_value(birth_date_node);
	sscanf(tmp, "%d-%d-%d", &year, &month, &day);

	start_date = *localtime(&today);
	start_date.tm_year = year - 1900;
	start_date.tm_mon = month;
	start_date.tm_mday = day;

	end_date = *localtime(&today);

	start_time = mktime(&start_date);
	end_time = mktime(&end_date);

	seconds = (int)difftime(end_time, start_time);
	seconds /= 60 * 60 * 24 * 365;

	rec->age = seconds;

	/* Gender */
	gender_node = json_object_get(result, "gender");

	rec->gender = json_integer_value(gender_node);

	/* Photo */
	photos_node = json_object_get(result, "photos");
	photo_node = json_array_get(photos_node, 0);
	processedFiles_node = json_object_get(photo_node, "processedFiles");

	for (i = 0, l = json_array_size(processedFiles_node); i < l; i++) {
		processedFile_node = json_array_get(processedFiles_node, i);
		height_node = json_object_get(processedFile_node, "height");

		if (json_integer_value(height_node) == 84) {
			break;
		}
	}

	url_node = json_object_get(processedFile_node, "url");

	tmp = json_string_value(url_node);
	url = malloc(sizeof(char) * (strlen(tmp) + 1));
	strcpy(url, tmp);

	jpeg2ascii(url, &rec->photo);

	/* Distance */
	distance_node = json_object_get(result, "distance_mi");

	rec->distance = json_integer_value(distance_node);

	/* Likes */
	likes_node = json_object_get(result, "common_like_count");

	rec->likes = json_integer_value(likes_node);

	/* Friends */
	friends_node = json_object_get(result, "common_friend_count");

	rec->friends = json_integer_value(friends_node);
}

static void get_pass(char *x_auth_token, char *id) {
	char *url = NULL, *data = NULL;

	url = malloc(sizeof(char) * (strlen(API_ROOT) + 7 + strlen(id)));
	sprintf(url, "%s/pass/%s", API_ROOT, id);

	send_request("GET", url, x_auth_token, NULL, &data);

	if (url) {
		free(url);
	}

	if (data) {
		free(data);
	}
}

static int get_like(char *x_auth_token, char *id) {
	int matched = 1;
	char *url = NULL, *data = NULL;
	json_t *root, *match;
	json_error_t error;

	url = malloc(sizeof(char) * (strlen(API_ROOT) + 7 + strlen(id)));
	sprintf(url, "%s/like/%s", API_ROOT, id);

	send_request("GET", url, x_auth_token, NULL, &data);
	root = json_loads(data, 0, &error);

	match = json_object_get(root, "match");

	if (match && json_is_false(match)) {
		matched = 0;
	}

	if (url) {
		free(url);
	}

	if (data) {
		free(data);
	}

	return matched;
}

static int get_token(char **facebook_token) {
	int rc = 0;
	size_t dotfile_length = 0;
	char *path = NULL, *home = getenv("HOME"), *dotfile = DOTFILE;
	FILE *dotfile_file = NULL;

	path = malloc(sizeof(char) * (strlen(home) + strlen(dotfile) + 2));
	sprintf(path, "%s/%s", home, dotfile);

	dotfile_file = fopen(path, "rb");

	if (!dotfile_file) {
		printf("Could not find %s\n", path);
		(void)fopen(path, "w+");

		rc = 1;
		goto cleanup;
	}

	(void)fseek(dotfile_file, 0, SEEK_END);
	dotfile_length = (size_t)ftell(dotfile_file);
	(void)fseek(dotfile_file, 0, SEEK_SET);

	*facebook_token = malloc(sizeof(char) * dotfile_length + 1);

	if (!*facebook_token) {
		rc = 1;
		goto cleanup;
	}

	if (fread(*facebook_token, 1, dotfile_length, dotfile_file) != dotfile_length) {
		rc = 1;
		goto cleanup;
	}

cleanup:
	if (dotfile_file) {
		(void)fclose(dotfile_file);
	}

	if (path) {
		free(path);
	}

	return rc;
}

static int set_token(char *facebook_token) {
	int rc = 0;
	size_t dotfile_length = 0;
	char *path = NULL, *home = getenv("HOME"), *dotfile = DOTFILE;
	FILE *dotfile_file = NULL;

	path = malloc(sizeof(char) * (strlen(home) + strlen(dotfile) + 2));
	sprintf(path, "%s/%s", home, dotfile);

	dotfile_file = fopen(path, "wb");
	dotfile_length = strlen(facebook_token);

	if (fwrite(facebook_token, 1, dotfile_length, dotfile_file) != dotfile_length) {
		rc = 1;
		goto cleanup;
	}

cleanup:
	if (dotfile_file) {
		(void)fclose(dotfile_file);
	}

	if (path) {
		free(path);
	}

	return rc;
}

static void usage(char *exec) {
	int length = (int)strlen(exec);

	printf("Usage: %s [-f,--facebook_token] \"<token>\"\n", exec);
	printf("%*s [-a,--auth]\n", length + 7, " ");
	printf("%*s [-h,--help]\n", length + 7, " ");
}

static void auth() {
	printf("%s\n", API_OAUTH);
}

int main(int argc, char *argv[]) {
	int rc = 1, i = 0, c = 0, action_like = 0, action_pass = 0;
	int rec_new = 0, state = 0, match = 0;
	size_t rec_count = 0, rec_max = 0;
	char *exec = NULL, *action = NULL, *facebook_token = NULL, *x_auth_token = NULL;
	json_t *results;
	struct rec rec;

	exec = argv[0];

	if (argc > 1) {
		action = argv[1];

		if (strcmp(action, "-f") == 0 || strcmp(action, "--facebook_token") == 0) {
			if (argc > 2) {
				facebook_token = malloc(sizeof(char) * strlen(argv[2]) + 1);
				strcpy(facebook_token, argv[2]);
			} else {
				usage(exec);

				rc = 1;
				goto cleanup;
			}
		} else if (strcmp(action, "-a") == 0 || strcmp(action, "--auth") == 0) {
			auth();

			rc = 1;
			goto cleanup;
		} else if (strcmp(action, "-h") == 0 || strcmp(action, "--help") == 0) {
			usage(exec);

			rc = 1;
			goto cleanup;
		} else {
			printf("Invalid action %s\n", action);
			goto cleanup;
		}
	} else {
		if (get_token(&facebook_token) != 0) {
			auth();

			rc = 1;
			goto cleanup;
		}
	}

	if (strlen(facebook_token) == 0) {
		printf("Missing token\n");
		printf("Please authorize:\n\n");
		auth();

		rc = 1;
		goto cleanup;
	}

	if (get_auth(facebook_token, &x_auth_token) != 0) {
		auth();

		rc = 1;
		goto cleanup;
	}

	(void)set_token(facebook_token);

	rec_count = get_recs(x_auth_token, &results);
	rec_max = rec_count;
	get_info(results, rec_max - rec_count, &rec);

	get_dims(&width, &height, &left);

	draw_init();

	while (1 == TRUE) {
		draw_border();

		if (rec_count == 0) {
			rec_count = get_recs(x_auth_token, &results);
			rec_max = rec_count;
		}

		if (rec_new == 1) {
			get_info(results, rec_max - rec_count, &rec);
			rec_new = 0;
		}

		if (state == 0) {
			draw_recs();
			draw_info(rec.name, rec.age);
			draw_photo(rec.photo);
		} else if (state == 1) {
			draw_bio(rec);
		}

		c = getch();

		if (c == ' ') { // SPACE
			draw_clear();
			state = 1 - state;
		}

		if (c == 'q') { // QUIT
			break;
		}

		if (state == 0) {
			if (c == '\033') {
				getch();

				c = getch();

				switch (c) {
				case 'C': // RIGHT
					if (action_like == 0) {
						i = 20;
						action_like = 1;

						rec_count--;
						rec_new = 1;
						match = get_like(x_auth_token, rec.id);
					}

					break;
				case 'D': // LEFT
					if (action_pass == 0) {
						i = 20;
						action_pass = 1;

						rec_count--;
						rec_new = 1;
						get_pass(x_auth_token, rec.id);
					}

					break;
				default:
					break;
				}
			}
		}

		if (i > 0) {
			i--;

			if (action_like == 1) {
				draw_like();

				if (match) {
					draw_match();
				}
			}

			if (action_pass == 1) {
				draw_pass();
			}
		} else {
			action_like = 0;
			action_pass = 0;

			clear_match();
		}

		draw_loop();
	}

	draw_end();

cleanup:
	if (facebook_token) {
		free(facebook_token);
	}

	return rc;
}