(function (factory) {
    if (typeof module === "object" && typeof module.exports === "object") {
        var v = factory(require, exports);
        if (v !== undefined) module.exports = v;
    }
    else if (typeof define === "function" && define.amd) {
        define(["require", "exports"], factory);
    }
})(function (require, exports) {
    "use strict";
    exports.__esModule = true;
    var diff_match_patch = (function () {
        function diff_match_patch() {
            this.Diff_Timeout = 1.0;
            this.Diff_EditCost = 4;
            this.Match_Threshold = 0.5;
            this.Match_Distance = 1000;
            this.Patch_DeleteThreshold = 0.5;
            this.Patch_Margin = 4;
            this.Match_MaxBits = 32;
        }
        diff_match_patch.prototype.diff_main = function (text1, text2, opt_checklines, opt_deadline) {
            if (typeof opt_deadline == 'undefined') {
                if (this.Diff_Timeout <= 0) {
                    opt_deadline = Number.MAX_VALUE;
                }
                else {
                    opt_deadline = (new Date).getTime() + this.Diff_Timeout * 1000;
                }
            }
            var deadline = opt_deadline;
            if (text1 == null || text2 == null) {
                throw new Error('Null input. (diff_main)');
            }
            if (text1 == text2) {
                if (text1) {
                    return [new Diff(exports.DIFF_EQUAL, text1)];
                }
                return [];
            }
            if (typeof opt_checklines == 'undefined') {
                opt_checklines = true;
            }
            var checklines = opt_checklines;
            var commonlength = this.diff_commonPrefix(text1, text2);
            var commonprefix = text1.substring(0, commonlength);
            text1 = text1.substring(commonlength);
            text2 = text2.substring(commonlength);
            commonlength = this.diff_commonSuffix(text1, text2);
            var commonsuffix = text1.substring(text1.length - commonlength);
            text1 = text1.substring(0, text1.length - commonlength);
            text2 = text2.substring(0, text2.length - commonlength);
            var diffs = this.diff_compute_(text1, text2, checklines, deadline);
            if (commonprefix) {
                diffs.unshift(new Diff(exports.DIFF_EQUAL, commonprefix));
            }
            if (commonsuffix) {
                diffs.push(new Diff(exports.DIFF_EQUAL, commonsuffix));
            }
            this.diff_cleanupMerge(diffs);
            return diffs;
        };
        ;
        diff_match_patch.prototype.diff_compute_ = function (text1, text2, checklines, deadline) {
            var diffs;
            if (!text1) {
                return [new Diff(exports.DIFF_INSERT, text2)];
            }
            if (!text2) {
                return [new Diff(exports.DIFF_DELETE, text1)];
            }
            var longtext = text1.length > text2.length ? text1 : text2;
            var shorttext = text1.length > text2.length ? text2 : text1;
            var i = longtext.indexOf(shorttext);
            if (i != -1) {
                diffs = [new Diff(exports.DIFF_INSERT, longtext.substring(0, i)),
                    new Diff(exports.DIFF_EQUAL, shorttext),
                    new Diff(exports.DIFF_INSERT, longtext.substring(i + shorttext.length))];
                if (text1.length > text2.length) {
                    diffs[0].operation = diffs[2].operation = exports.DIFF_DELETE;
                }
                return diffs;
            }
            if (shorttext.length == 1) {
                return [new Diff(exports.DIFF_DELETE, text1),
                    new Diff(exports.DIFF_INSERT, text2)];
            }
            var hm = this.diff_halfMatch_(text1, text2);
            if (hm) {
                var text1_a = hm[0];
                var text1_b = hm[1];
                var text2_a = hm[2];
                var text2_b = hm[3];
                var mid_common = hm[4];
                var diffs_a = this.diff_main(text1_a, text2_a, checklines, deadline);
                var diffs_b = this.diff_main(text1_b, text2_b, checklines, deadline);
                return diffs_a.concat([new Diff(exports.DIFF_EQUAL, mid_common)], diffs_b);
            }
            if (checklines && text1.length > 100 && text2.length > 100) {
                return this.diff_lineMode_(text1, text2, deadline);
            }
            return this.diff_bisect_(text1, text2, deadline);
        };
        ;
        diff_match_patch.prototype.diff_lineMode_ = function (text1, text2, deadline) {
            var a = this.diff_linesToChars_(text1, text2);
            text1 = a.chars1;
            text2 = a.chars2;
            var linearray = a.lineArray;
            var diffs = this.diff_main(text1, text2, false, deadline);
            this.diff_charsToLines_(diffs, linearray);
            this.diff_cleanupSemantic(diffs);
            diffs.push(new Diff(exports.DIFF_EQUAL, ''));
            var pointer = 0;
            var count_delete = 0;
            var count_insert = 0;
            var text_delete = '';
            var text_insert = '';
            while (pointer < diffs.length) {
                switch (diffs[pointer].operation) {
                    case exports.DIFF_INSERT:
                        count_insert++;
                        text_insert += diffs[pointer].text;
                        break;
                    case exports.DIFF_DELETE:
                        count_delete++;
                        text_delete += diffs[pointer].text;
                        break;
                    case exports.DIFF_EQUAL:
                        if (count_delete >= 1 && count_insert >= 1) {
                            diffs.splice(pointer - count_delete - count_insert, count_delete + count_insert);
                            pointer = pointer - count_delete - count_insert;
                            var subDiff = this.diff_main(text_delete, text_insert, false, deadline);
                            for (var j = subDiff.length - 1; j >= 0; j--) {
                                diffs.splice(pointer, 0, subDiff[j]);
                            }
                            pointer = pointer + subDiff.length;
                        }
                        count_insert = 0;
                        count_delete = 0;
                        text_delete = '';
                        text_insert = '';
                        break;
                }
                pointer++;
            }
            diffs.pop();
            return diffs;
        };
        ;
        diff_match_patch.prototype.diff_bisect_ = function (text1, text2, deadline) {
            var text1_length = text1.length;
            var text2_length = text2.length;
            var max_d = Math.ceil((text1_length + text2_length) / 2);
            var v_offset = max_d;
            var v_length = 2 * max_d;
            var v1 = new Array(v_length);
            var v2 = new Array(v_length);
            for (var x = 0; x < v_length; x++) {
                v1[x] = -1;
                v2[x] = -1;
            }
            v1[v_offset + 1] = 0;
            v2[v_offset + 1] = 0;
            var delta = text1_length - text2_length;
            var front = (delta % 2 != 0);
            var k1start = 0;
            var k1end = 0;
            var k2start = 0;
            var k2end = 0;
            for (var d = 0; d < max_d; d++) {
                if ((new Date()).getTime() > deadline) {
                    break;
                }
                for (var k1 = -d + k1start; k1 <= d - k1end; k1 += 2) {
                    var k1_offset = v_offset + k1;
                    var x1;
                    if (k1 == -d || (k1 != d && v1[k1_offset - 1] < v1[k1_offset + 1])) {
                        x1 = v1[k1_offset + 1];
                    }
                    else {
                        x1 = v1[k1_offset - 1] + 1;
                    }
                    var y1 = x1 - k1;
                    while (x1 < text1_length && y1 < text2_length &&
                        text1.charAt(x1) == text2.charAt(y1)) {
                        x1++;
                        y1++;
                    }
                    v1[k1_offset] = x1;
                    if (x1 > text1_length) {
                        k1end += 2;
                    }
                    else if (y1 > text2_length) {
                        k1start += 2;
                    }
                    else if (front) {
                        var k2_offset = v_offset + delta - k1;
                        if (k2_offset >= 0 && k2_offset < v_length && v2[k2_offset] != -1) {
                            var x2 = text1_length - v2[k2_offset];
                            if (x1 >= x2) {
                                return this.diff_bisectSplit_(text1, text2, x1, y1, deadline);
                            }
                        }
                    }
                }
                for (var k2 = -d + k2start; k2 <= d - k2end; k2 += 2) {
                    var k2_offset = v_offset + k2;
                    var x2;
                    if (k2 == -d || (k2 != d && v2[k2_offset - 1] < v2[k2_offset + 1])) {
                        x2 = v2[k2_offset + 1];
                    }
                    else {
                        x2 = v2[k2_offset - 1] + 1;
                    }
                    var y2 = x2 - k2;
                    while (x2 < text1_length && y2 < text2_length &&
                        text1.charAt(text1_length - x2 - 1) ==
                            text2.charAt(text2_length - y2 - 1)) {
                        x2++;
                        y2++;
                    }
                    v2[k2_offset] = x2;
                    if (x2 > text1_length) {
                        k2end += 2;
                    }
                    else if (y2 > text2_length) {
                        k2start += 2;
                    }
                    else if (!front) {
                        var k1_offset = v_offset + delta - k2;
                        if (k1_offset >= 0 && k1_offset < v_length && v1[k1_offset] != -1) {
                            var x1 = v1[k1_offset];
                            var y1 = v_offset + x1 - k1_offset;
                            x2 = text1_length - x2;
                            if (x1 >= x2) {
                                return this.diff_bisectSplit_(text1, text2, x1, y1, deadline);
                            }
                        }
                    }
                }
            }
            return [new Diff(exports.DIFF_DELETE, text1),
                new Diff(exports.DIFF_INSERT, text2)];
        };
        ;
        diff_match_patch.prototype.diff_bisectSplit_ = function (text1, text2, x, y, deadline) {
            var text1a = text1.substring(0, x);
            var text2a = text2.substring(0, y);
            var text1b = text1.substring(x);
            var text2b = text2.substring(y);
            var diffs = this.diff_main(text1a, text2a, false, deadline);
            var diffsb = this.diff_main(text1b, text2b, false, deadline);
            return diffs.concat(diffsb);
        };
        ;
        diff_match_patch.prototype.diff_linesToChars_ = function (text1, text2) {
            var lineArray = [];
            var lineHash = {};
            lineArray[0] = '';
            function diff_linesToCharsMunge_(text) {
                var chars = '';
                var lineStart = 0;
                var lineEnd = -1;
                var lineArrayLength = lineArray.length;
                while (lineEnd < text.length - 1) {
                    lineEnd = text.indexOf('\n', lineStart);
                    if (lineEnd == -1) {
                        lineEnd = text.length - 1;
                    }
                    var line = text.substring(lineStart, lineEnd + 1);
                    if (lineHash.hasOwnProperty ? lineHash.hasOwnProperty(line) :
                        (lineHash[line] !== undefined)) {
                        chars += String.fromCharCode(lineHash[line]);
                    }
                    else {
                        if (lineArrayLength == maxLines) {
                            line = text.substring(lineStart);
                            lineEnd = text.length;
                        }
                        chars += String.fromCharCode(lineArrayLength);
                        lineHash[line] = lineArrayLength;
                        lineArray[lineArrayLength++] = line;
                    }
                    lineStart = lineEnd + 1;
                }
                return chars;
            }
            var maxLines = 40000;
            var chars1 = diff_linesToCharsMunge_(text1);
            maxLines = 65535;
            var chars2 = diff_linesToCharsMunge_(text2);
            return { chars1: chars1, chars2: chars2, lineArray: lineArray };
        };
        ;
        diff_match_patch.prototype.diff_charsToLines_ = function (diffs, lineArray) {
            for (var i = 0; i < diffs.length; i++) {
                var chars = diffs[i].text;
                var text = [];
                for (var j = 0; j < chars.length; j++) {
                    text[j] = lineArray[chars.charCodeAt(j)];
                }
                diffs[i].text = text.join('');
            }
        };
        ;
        diff_match_patch.prototype.diff_commonPrefix = function (text1, text2) {
            if (!text1 || !text2 || text1.charAt(0) != text2.charAt(0)) {
                return 0;
            }
            var pointermin = 0;
            var pointermax = Math.min(text1.length, text2.length);
            var pointermid = pointermax;
            var pointerstart = 0;
            while (pointermin < pointermid) {
                if (text1.substring(pointerstart, pointermid) ==
                    text2.substring(pointerstart, pointermid)) {
                    pointermin = pointermid;
                    pointerstart = pointermin;
                }
                else {
                    pointermax = pointermid;
                }
                pointermid = Math.floor((pointermax - pointermin) / 2 + pointermin);
            }
            return pointermid;
        };
        ;
        diff_match_patch.prototype.diff_commonSuffix = function (text1, text2) {
            if (!text1 || !text2 ||
                text1.charAt(text1.length - 1) != text2.charAt(text2.length - 1)) {
                return 0;
            }
            var pointermin = 0;
            var pointermax = Math.min(text1.length, text2.length);
            var pointermid = pointermax;
            var pointerend = 0;
            while (pointermin < pointermid) {
                if (text1.substring(text1.length - pointermid, text1.length - pointerend) ==
                    text2.substring(text2.length - pointermid, text2.length - pointerend)) {
                    pointermin = pointermid;
                    pointerend = pointermin;
                }
                else {
                    pointermax = pointermid;
                }
                pointermid = Math.floor((pointermax - pointermin) / 2 + pointermin);
            }
            return pointermid;
        };
        ;
        diff_match_patch.prototype.diff_commonOverlap_ = function (text1, text2) {
            var text1_length = text1.length;
            var text2_length = text2.length;
            if (text1_length == 0 || text2_length == 0) {
                return 0;
            }
            if (text1_length > text2_length) {
                text1 = text1.substring(text1_length - text2_length);
            }
            else if (text1_length < text2_length) {
                text2 = text2.substring(0, text1_length);
            }
            var text_length = Math.min(text1_length, text2_length);
            if (text1 == text2) {
                return text_length;
            }
            var best = 0;
            var length = 1;
            while (true) {
                var pattern = text1.substring(text_length - length);
                var found = text2.indexOf(pattern);
                if (found == -1) {
                    return best;
                }
                length += found;
                if (found == 0 || text1.substring(text_length - length) ==
                    text2.substring(0, length)) {
                    best = length;
                    length++;
                }
            }
        };
        ;
        diff_match_patch.prototype.diff_halfMatch_ = function (text1, text2) {
            if (this.Diff_Timeout <= 0) {
                return null;
            }
            var longtext = text1.length > text2.length ? text1 : text2;
            var shorttext = text1.length > text2.length ? text2 : text1;
            if (longtext.length < 4 || shorttext.length * 2 < longtext.length) {
                return null;
            }
            var dmp = this;
            function diff_halfMatchI_(longtext, shorttext, i) {
                var seed = longtext.substring(i, i + Math.floor(longtext.length / 4));
                var j = -1;
                var best_common = '';
                var best_longtext_a = '';
                var best_longtext_b = '';
                var best_shorttext_a = '';
                var best_shorttext_b = '';
                while ((j = shorttext.indexOf(seed, j + 1)) != -1) {
                    var prefixLength = dmp.diff_commonPrefix(longtext.substring(i), shorttext.substring(j));
                    var suffixLength = dmp.diff_commonSuffix(longtext.substring(0, i), shorttext.substring(0, j));
                    if (best_common.length < suffixLength + prefixLength) {
                        best_common = shorttext.substring(j - suffixLength, j) +
                            shorttext.substring(j, j + prefixLength);
                        best_longtext_a = longtext.substring(0, i - suffixLength);
                        best_longtext_b = longtext.substring(i + prefixLength);
                        best_shorttext_a = shorttext.substring(0, j - suffixLength);
                        best_shorttext_b = shorttext.substring(j + prefixLength);
                    }
                }
                if (best_common.length * 2 >= longtext.length) {
                    return [best_longtext_a, best_longtext_b,
                        best_shorttext_a, best_shorttext_b, best_common];
                }
                else {
                    return null;
                }
            }
            var hm1 = diff_halfMatchI_(longtext, shorttext, Math.ceil(longtext.length / 4));
            var hm2 = diff_halfMatchI_(longtext, shorttext, Math.ceil(longtext.length / 2));
            var hm;
            if (!hm1 && !hm2) {
                return null;
            }
            else if (!hm2) {
                hm = hm1;
            }
            else if (!hm1) {
                hm = hm2;
            }
            else {
                hm = hm1[4].length > hm2[4].length ? hm1 : hm2;
            }
            var text1_a, text1_b, text2_a, text2_b;
            if (text1.length > text2.length) {
                text1_a = hm[0];
                text1_b = hm[1];
                text2_a = hm[2];
                text2_b = hm[3];
            }
            else {
                text2_a = hm[0];
                text2_b = hm[1];
                text1_a = hm[2];
                text1_b = hm[3];
            }
            var mid_common = hm[4];
            return [text1_a, text1_b, text2_a, text2_b, mid_common];
        };
        ;
        diff_match_patch.prototype.diff_cleanupSemantic = function (diffs) {
            var changes = false;
            var equalities = [];
            var equalitiesLength = 0;
            var lastEquality = null;
            var pointer = 0;
            var length_insertions1 = 0;
            var length_deletions1 = 0;
            var length_insertions2 = 0;
            var length_deletions2 = 0;
            while (pointer < diffs.length) {
                if (diffs[pointer].operation == exports.DIFF_EQUAL) {
                    equalities[equalitiesLength++] = pointer;
                    length_insertions1 = length_insertions2;
                    length_deletions1 = length_deletions2;
                    length_insertions2 = 0;
                    length_deletions2 = 0;
                    lastEquality = diffs[pointer].text;
                }
                else {
                    if (diffs[pointer].operation == exports.DIFF_INSERT) {
                        length_insertions2 += diffs[pointer].text.length;
                    }
                    else {
                        length_deletions2 += diffs[pointer].text.length;
                    }
                    if (lastEquality && (lastEquality.length <=
                        Math.max(length_insertions1, length_deletions1)) &&
                        (lastEquality.length <= Math.max(length_insertions2, length_deletions2))) {
                        diffs.splice(equalities[equalitiesLength - 1], 0, new Diff(exports.DIFF_DELETE, lastEquality));
                        diffs[equalities[equalitiesLength - 1] + 1].operation = exports.DIFF_INSERT;
                        equalitiesLength--;
                        equalitiesLength--;
                        pointer = equalitiesLength > 0 ? equalities[equalitiesLength - 1] : -1;
                        length_insertions1 = 0;
                        length_deletions1 = 0;
                        length_insertions2 = 0;
                        length_deletions2 = 0;
                        lastEquality = null;
                        changes = true;
                    }
                }
                pointer++;
            }
            if (changes) {
                this.diff_cleanupMerge(diffs);
            }
            this.diff_cleanupSemanticLossless(diffs);
            pointer = 1;
            while (pointer < diffs.length) {
                if (diffs[pointer - 1].operation == exports.DIFF_DELETE &&
                    diffs[pointer].operation == exports.DIFF_INSERT) {
                    var deletion = diffs[pointer - 1].text;
                    var insertion = diffs[pointer].text;
                    var overlap_length1 = this.diff_commonOverlap_(deletion, insertion);
                    var overlap_length2 = this.diff_commonOverlap_(insertion, deletion);
                    if (overlap_length1 >= overlap_length2) {
                        if (overlap_length1 >= deletion.length / 2 ||
                            overlap_length1 >= insertion.length / 2) {
                            diffs.splice(pointer, 0, new Diff(exports.DIFF_EQUAL, insertion.substring(0, overlap_length1)));
                            diffs[pointer - 1].text =
                                deletion.substring(0, deletion.length - overlap_length1);
                            diffs[pointer + 1].text = insertion.substring(overlap_length1);
                            pointer++;
                        }
                    }
                    else {
                        if (overlap_length2 >= deletion.length / 2 ||
                            overlap_length2 >= insertion.length / 2) {
                            diffs.splice(pointer, 0, new Diff(exports.DIFF_EQUAL, deletion.substring(0, overlap_length2)));
                            diffs[pointer - 1].operation = exports.DIFF_INSERT;
                            diffs[pointer - 1].text =
                                insertion.substring(0, insertion.length - overlap_length2);
                            diffs[pointer + 1].operation = exports.DIFF_DELETE;
                            diffs[pointer + 1].text =
                                deletion.substring(overlap_length2);
                            pointer++;
                        }
                    }
                    pointer++;
                }
                pointer++;
            }
        };
        ;
        diff_match_patch.prototype.diff_cleanupSemanticLossless = function (diffs) {
            function diff_cleanupSemanticScore_(one, two) {
                if (!one || !two) {
                    return 6;
                }
                var char1 = one.charAt(one.length - 1);
                var char2 = two.charAt(0);
                var nonAlphaNumeric1 = char1.match(diff_match_patch.nonAlphaNumericRegex_);
                var nonAlphaNumeric2 = char2.match(diff_match_patch.nonAlphaNumericRegex_);
                var whitespace1 = nonAlphaNumeric1 &&
                    char1.match(diff_match_patch.whitespaceRegex_);
                var whitespace2 = nonAlphaNumeric2 &&
                    char2.match(diff_match_patch.whitespaceRegex_);
                var lineBreak1 = whitespace1 &&
                    char1.match(diff_match_patch.linebreakRegex_);
                var lineBreak2 = whitespace2 &&
                    char2.match(diff_match_patch.linebreakRegex_);
                var blankLine1 = lineBreak1 &&
                    one.match(diff_match_patch.blanklineEndRegex_);
                var blankLine2 = lineBreak2 &&
                    two.match(diff_match_patch.blanklineStartRegex_);
                if (blankLine1 || blankLine2) {
                    return 5;
                }
                else if (lineBreak1 || lineBreak2) {
                    return 4;
                }
                else if (nonAlphaNumeric1 && !whitespace1 && whitespace2) {
                    return 3;
                }
                else if (whitespace1 || whitespace2) {
                    return 2;
                }
                else if (nonAlphaNumeric1 || nonAlphaNumeric2) {
                    return 1;
                }
                return 0;
            }
            var pointer = 1;
            while (pointer < diffs.length - 1) {
                if (diffs[pointer - 1].operation == exports.DIFF_EQUAL &&
                    diffs[pointer + 1].operation == exports.DIFF_EQUAL) {
                    var equality1 = diffs[pointer - 1].text;
                    var edit = diffs[pointer].text;
                    var equality2 = diffs[pointer + 1].text;
                    var commonOffset = this.diff_commonSuffix(equality1, edit);
                    if (commonOffset) {
                        var commonString = edit.substring(edit.length - commonOffset);
                        equality1 = equality1.substring(0, equality1.length - commonOffset);
                        edit = commonString + edit.substring(0, edit.length - commonOffset);
                        equality2 = commonString + equality2;
                    }
                    var bestEquality1 = equality1;
                    var bestEdit = edit;
                    var bestEquality2 = equality2;
                    var bestScore = diff_cleanupSemanticScore_(equality1, edit) +
                        diff_cleanupSemanticScore_(edit, equality2);
                    while (edit.charAt(0) === equality2.charAt(0)) {
                        equality1 += edit.charAt(0);
                        edit = edit.substring(1) + equality2.charAt(0);
                        equality2 = equality2.substring(1);
                        var score = diff_cleanupSemanticScore_(equality1, edit) +
                            diff_cleanupSemanticScore_(edit, equality2);
                        if (score >= bestScore) {
                            bestScore = score;
                            bestEquality1 = equality1;
                            bestEdit = edit;
                            bestEquality2 = equality2;
                        }
                    }
                    if (diffs[pointer - 1].text != bestEquality1) {
                        if (bestEquality1) {
                            diffs[pointer - 1].text = bestEquality1;
                        }
                        else {
                            diffs.splice(pointer - 1, 1);
                            pointer--;
                        }
                        diffs[pointer].text = bestEdit;
                        if (bestEquality2) {
                            diffs[pointer + 1].text = bestEquality2;
                        }
                        else {
                            diffs.splice(pointer + 1, 1);
                            pointer--;
                        }
                    }
                }
                pointer++;
            }
        };
        ;
        diff_match_patch.prototype.diff_cleanupEfficiency = function (diffs) {
            var changes = false;
            var equalities = [];
            var equalitiesLength = 0;
            var lastEquality = null;
            var pointer = 0;
            var pre_ins = 0;
            var pre_del = 0;
            var post_ins = 0;
            var post_del = 0;
            while (pointer < diffs.length) {
                if (diffs[pointer].operation == exports.DIFF_EQUAL) {
                    if (diffs[pointer].text.length < this.Diff_EditCost &&
                        (post_ins || post_del)) {
                        equalities[equalitiesLength++] = pointer;
                        pre_ins = post_ins;
                        pre_del = post_del;
                        lastEquality = diffs[pointer].text;
                    }
                    else {
                        equalitiesLength = 0;
                        lastEquality = null;
                    }
                    post_ins = post_del = 0;
                }
                else {
                    if (diffs[pointer].operation == exports.DIFF_DELETE) {
                        post_del = 1;
                    }
                    else {
                        post_ins = 1;
                    }
                    if (lastEquality && ((pre_ins && pre_del && post_ins && post_del) ||
                        ((lastEquality.length < this.Diff_EditCost / 2) &&
                            (pre_ins + pre_del + post_ins + post_del) == 3))) {
                        diffs.splice(equalities[equalitiesLength - 1], 0, new Diff(exports.DIFF_DELETE, lastEquality));
                        diffs[equalities[equalitiesLength - 1] + 1].operation = exports.DIFF_INSERT;
                        equalitiesLength--;
                        lastEquality = null;
                        if (pre_ins && pre_del) {
                            post_ins = post_del = 1;
                            equalitiesLength = 0;
                        }
                        else {
                            equalitiesLength--;
                            pointer = equalitiesLength > 0 ?
                                equalities[equalitiesLength - 1] : -1;
                            post_ins = post_del = 0;
                        }
                        changes = true;
                    }
                }
                pointer++;
            }
            if (changes) {
                this.diff_cleanupMerge(diffs);
            }
        };
        ;
        diff_match_patch.prototype.diff_cleanupMerge = function (diffs) {
            diffs.push(new Diff(exports.DIFF_EQUAL, ''));
            var pointer = 0;
            var count_delete = 0;
            var count_insert = 0;
            var text_delete = '';
            var text_insert = '';
            var commonlength;
            while (pointer < diffs.length) {
                switch (diffs[pointer].operation) {
                    case exports.DIFF_INSERT:
                        count_insert++;
                        text_insert += diffs[pointer].text;
                        pointer++;
                        break;
                    case exports.DIFF_DELETE:
                        count_delete++;
                        text_delete += diffs[pointer].text;
                        pointer++;
                        break;
                    case exports.DIFF_EQUAL:
                        if (count_delete + count_insert > 1) {
                            if (count_delete !== 0 && count_insert !== 0) {
                                commonlength = this.diff_commonPrefix(text_insert, text_delete);
                                if (commonlength !== 0) {
                                    if ((pointer - count_delete - count_insert) > 0 &&
                                        diffs[pointer - count_delete - count_insert - 1].operation ==
                                            exports.DIFF_EQUAL) {
                                        diffs[pointer - count_delete - count_insert - 1].text +=
                                            text_insert.substring(0, commonlength);
                                    }
                                    else {
                                        diffs.splice(0, 0, new Diff(exports.DIFF_EQUAL, text_insert.substring(0, commonlength)));
                                        pointer++;
                                    }
                                    text_insert = text_insert.substring(commonlength);
                                    text_delete = text_delete.substring(commonlength);
                                }
                                commonlength = this.diff_commonSuffix(text_insert, text_delete);
                                if (commonlength !== 0) {
                                    diffs[pointer].text = text_insert.substring(text_insert.length -
                                        commonlength) + diffs[pointer].text;
                                    text_insert = text_insert.substring(0, text_insert.length -
                                        commonlength);
                                    text_delete = text_delete.substring(0, text_delete.length -
                                        commonlength);
                                }
                            }
                            pointer -= count_delete + count_insert;
                            diffs.splice(pointer, count_delete + count_insert);
                            if (text_delete.length) {
                                diffs.splice(pointer, 0, new Diff(exports.DIFF_DELETE, text_delete));
                                pointer++;
                            }
                            if (text_insert.length) {
                                diffs.splice(pointer, 0, new Diff(exports.DIFF_INSERT, text_insert));
                                pointer++;
                            }
                            pointer++;
                        }
                        else if (pointer !== 0 && diffs[pointer - 1].operation == exports.DIFF_EQUAL) {
                            diffs[pointer - 1].text += diffs[pointer].text;
                            diffs.splice(pointer, 1);
                        }
                        else {
                            pointer++;
                        }
                        count_insert = 0;
                        count_delete = 0;
                        text_delete = '';
                        text_insert = '';
                        break;
                }
            }
            if (diffs[diffs.length - 1].text === '') {
                diffs.pop();
            }
            var changes = false;
            pointer = 1;
            while (pointer < diffs.length - 1) {
                if (diffs[pointer - 1].operation == exports.DIFF_EQUAL &&
                    diffs[pointer + 1].operation == exports.DIFF_EQUAL) {
                    if (diffs[pointer].text.substring(diffs[pointer].text.length -
                        diffs[pointer - 1].text.length) == diffs[pointer - 1].text) {
                        diffs[pointer].text = diffs[pointer - 1].text +
                            diffs[pointer].text.substring(0, diffs[pointer].text.length -
                                diffs[pointer - 1].text.length);
                        diffs[pointer + 1].text = diffs[pointer - 1].text + diffs[pointer + 1].text;
                        diffs.splice(pointer - 1, 1);
                        changes = true;
                    }
                    else if (diffs[pointer].text.substring(0, diffs[pointer + 1].text.length) ==
                        diffs[pointer + 1].text) {
                        diffs[pointer - 1].text += diffs[pointer + 1].text;
                        diffs[pointer].text =
                            diffs[pointer].text.substring(diffs[pointer + 1].text.length) +
                                diffs[pointer + 1].text;
                        diffs.splice(pointer + 1, 1);
                        changes = true;
                    }
                }
                pointer++;
            }
            if (changes) {
                this.diff_cleanupMerge(diffs);
            }
        };
        ;
        diff_match_patch.prototype.diff_xIndex = function (diffs, loc) {
            var chars1 = 0;
            var chars2 = 0;
            var last_chars1 = 0;
            var last_chars2 = 0;
            var x;
            for (x = 0; x < diffs.length; x++) {
                if (diffs[x].operation !== exports.DIFF_INSERT) {
                    chars1 += diffs[x].text.length;
                }
                if (diffs[x].operation !== exports.DIFF_DELETE) {
                    chars2 += diffs[x].text.length;
                }
                if (chars1 > loc) {
                    break;
                }
                last_chars1 = chars1;
                last_chars2 = chars2;
            }
            if (diffs.length != x && diffs[x].operation === exports.DIFF_DELETE) {
                return last_chars2;
            }
            return last_chars2 + (loc - last_chars1);
        };
        ;
        diff_match_patch.prototype.diff_prettyHtml = function (diffs) {
            var html = [];
            var pattern_amp = /&/g;
            var pattern_lt = /</g;
            var pattern_gt = />/g;
            var pattern_para = /\n/g;
            for (var x = 0; x < diffs.length; x++) {
                var op = diffs[x].operation;
                var data = diffs[x].text;
                var text = data.replace(pattern_amp, '&amp;').replace(pattern_lt, '&lt;')
                    .replace(pattern_gt, '&gt;').replace(pattern_para, '&para;<br>');
                switch (op) {
                    case exports.DIFF_INSERT:
                        html[x] = '<ins style="background:#e6ffe6;">' + text + '</ins>';
                        break;
                    case exports.DIFF_DELETE:
                        html[x] = '<del style="background:#ffe6e6;">' + text + '</del>';
                        break;
                    case exports.DIFF_EQUAL:
                        html[x] = '<span>' + text + '</span>';
                        break;
                }
            }
            return html.join('');
        };
        ;
        diff_match_patch.prototype.diff_text1 = function (diffs) {
            var text = [];
            for (var x = 0; x < diffs.length; x++) {
                if (diffs[x].operation !== exports.DIFF_INSERT) {
                    text[x] = diffs[x].text;
                }
            }
            return text.join('');
        };
        ;
        diff_match_patch.prototype.diff_text2 = function (diffs) {
            var text = [];
            for (var x = 0; x < diffs.length; x++) {
                if (diffs[x].operation !== exports.DIFF_DELETE) {
                    text[x] = diffs[x].text;
                }
            }
            return text.join('');
        };
        ;
        diff_match_patch.prototype.diff_levenshtein = function (diffs) {
            var levenshtein = 0;
            var insertions = 0;
            var deletions = 0;
            for (var x = 0; x < diffs.length; x++) {
                var op = diffs[x].operation;
                var data = diffs[x].text;
                switch (op) {
                    case exports.DIFF_INSERT:
                        insertions += data.length;
                        break;
                    case exports.DIFF_DELETE:
                        deletions += data.length;
                        break;
                    case exports.DIFF_EQUAL:
                        levenshtein += Math.max(insertions, deletions);
                        insertions = 0;
                        deletions = 0;
                        break;
                }
            }
            levenshtein += Math.max(insertions, deletions);
            return levenshtein;
        };
        ;
        diff_match_patch.prototype.diff_toDelta = function (diffs) {
            var text = [];
            for (var x = 0; x < diffs.length; x++) {
                switch (diffs[x].operation) {
                    case exports.DIFF_INSERT:
                        text[x] = '+' + encodeURI(diffs[x].text);
                        break;
                    case exports.DIFF_DELETE:
                        text[x] = '-' + diffs[x].text.length;
                        break;
                    case exports.DIFF_EQUAL:
                        text[x] = '=' + diffs[x].text.length;
                        break;
                }
            }
            return text.join('\t').replace(/%20/g, ' ');
        };
        ;
        diff_match_patch.prototype.diff_fromDelta = function (text1, delta) {
            var diffs = [];
            var diffsLength = 0;
            var pointer = 0;
            var tokens = delta.split(/\t/g);
            for (var x = 0; x < tokens.length; x++) {
                var param = tokens[x].substring(1);
                switch (tokens[x].charAt(0)) {
                    case '+':
                        try {
                            diffs[diffsLength++] =
                                new Diff(exports.DIFF_INSERT, decodeURI(param));
                        }
                        catch (ex) {
                            throw new Error('Illegal escape in diff_fromDelta: ' + param);
                        }
                        break;
                    case '-':
                    case '=':
                        var n = parseInt(param, 10);
                        if (isNaN(n) || n < 0) {
                            throw new Error('Invalid number in diff_fromDelta: ' + param);
                        }
                        var text = text1.substring(pointer, pointer += n);
                        if (tokens[x].charAt(0) == '=') {
                            diffs[diffsLength++] = new Diff(exports.DIFF_EQUAL, text);
                        }
                        else {
                            diffs[diffsLength++] = new Diff(exports.DIFF_DELETE, text);
                        }
                        break;
                    default:
                        if (tokens[x]) {
                            throw new Error('Invalid diff operation in diff_fromDelta: ' +
                                tokens[x]);
                        }
                }
            }
            if (pointer != text1.length) {
                throw new Error('Delta length (' + pointer +
                    ') does not equal source text length (' + text1.length + ').');
            }
            return diffs;
        };
        ;
        diff_match_patch.prototype.match_main = function (text, pattern, loc) {
            if (text == null || pattern == null || loc == null) {
                throw new Error('Null input. (match_main)');
            }
            loc = Math.max(0, Math.min(loc, text.length));
            if (text == pattern) {
                return 0;
            }
            else if (!text.length) {
                return -1;
            }
            else if (text.substring(loc, loc + pattern.length) == pattern) {
                return loc;
            }
            else {
                return this.match_bitap_(text, pattern, loc);
            }
        };
        ;
        diff_match_patch.prototype.match_bitap_ = function (text, pattern, loc) {
            if (pattern.length > this.Match_MaxBits) {
                throw new Error('Pattern too long for this browser.');
            }
            var s = this.match_alphabet_(pattern);
            var dmp = this;
            function match_bitapScore_(e, x) {
                var accuracy = e / pattern.length;
                var proximity = Math.abs(loc - x);
                if (!dmp.Match_Distance) {
                    return proximity ? 1.0 : accuracy;
                }
                return accuracy + (proximity / dmp.Match_Distance);
            }
            var score_threshold = this.Match_Threshold;
            var best_loc = text.indexOf(pattern, loc);
            if (best_loc != -1) {
                score_threshold = Math.min(match_bitapScore_(0, best_loc), score_threshold);
                best_loc = text.lastIndexOf(pattern, loc + pattern.length);
                if (best_loc != -1) {
                    score_threshold =
                        Math.min(match_bitapScore_(0, best_loc), score_threshold);
                }
            }
            var matchmask = 1 << (pattern.length - 1);
            best_loc = -1;
            var bin_min, bin_mid;
            var bin_max = pattern.length + text.length;
            var last_rd = [];
            for (var d = 0; d < pattern.length; d++) {
                bin_min = 0;
                bin_mid = bin_max;
                while (bin_min < bin_mid) {
                    if (match_bitapScore_(d, loc + bin_mid) <= score_threshold) {
                        bin_min = bin_mid;
                    }
                    else {
                        bin_max = bin_mid;
                    }
                    bin_mid = Math.floor((bin_max - bin_min) / 2 + bin_min);
                }
                bin_max = bin_mid;
                var start = Math.max(1, loc - bin_mid + 1);
                var finish = Math.min(loc + bin_mid, text.length) + pattern.length;
                var rd = Array(finish + 2);
                rd[finish + 1] = (1 << d) - 1;
                for (var j = finish; j >= start; j--) {
                    var charMatch = s[text.charAt(j - 1)];
                    if (d === 0) {
                        rd[j] = ((rd[j + 1] << 1) | 1) & charMatch;
                    }
                    else {
                        rd[j] = (((rd[j + 1] << 1) | 1) & charMatch) |
                            (((last_rd[j + 1] | last_rd[j]) << 1) | 1) |
                            last_rd[j + 1];
                    }
                    if (rd[j] & matchmask) {
                        var score = match_bitapScore_(d, j - 1);
                        if (score <= score_threshold) {
                            score_threshold = score;
                            best_loc = j - 1;
                            if (best_loc > loc) {
                                start = Math.max(1, 2 * loc - best_loc);
                            }
                            else {
                                break;
                            }
                        }
                    }
                }
                if (match_bitapScore_(d + 1, loc) > score_threshold) {
                    break;
                }
                last_rd = rd;
            }
            return best_loc;
        };
        ;
        diff_match_patch.prototype.match_alphabet_ = function (pattern) {
            var s = {};
            for (var i = 0; i < pattern.length; i++) {
                s[pattern.charAt(i)] = 0;
            }
            for (var i = 0; i < pattern.length; i++) {
                s[pattern.charAt(i)] |= 1 << (pattern.length - i - 1);
            }
            return s;
        };
        ;
        diff_match_patch.prototype.patch_addContext_ = function (patch, text) {
            if (text.length == 0) {
                return;
            }
            if (patch.start2 === null) {
                throw Error('patch not initialized');
            }
            var pattern = text.substring(patch.start2, patch.start2 + patch.length1);
            var padding = 0;
            while (text.indexOf(pattern) != text.lastIndexOf(pattern) &&
                pattern.length < this.Match_MaxBits - this.Patch_Margin -
                    this.Patch_Margin) {
                padding += this.Patch_Margin;
                pattern = text.substring(patch.start2 - padding, patch.start2 + patch.length1 + padding);
            }
            padding += this.Patch_Margin;
            var prefix = text.substring(patch.start2 - padding, patch.start2);
            if (prefix) {
                patch.diffs.unshift(new Diff(exports.DIFF_EQUAL, prefix));
            }
            var suffix = text.substring(patch.start2 + patch.length1, patch.start2 + patch.length1 + padding);
            if (suffix) {
                patch.diffs.push(new Diff(exports.DIFF_EQUAL, suffix));
            }
            patch.start1 -= prefix.length;
            patch.start2 -= prefix.length;
            patch.length1 += prefix.length + suffix.length;
            patch.length2 += prefix.length + suffix.length;
        };
        ;
        diff_match_patch.prototype.patch_make = function (a, opt_b, opt_c) {
            var text1, diffs;
            if (typeof a == 'string' && typeof opt_b == 'string' &&
                typeof opt_c == 'undefined') {
                text1 = (a);
                diffs = this.diff_main(text1, (opt_b), true);
                if (diffs.length > 2) {
                    this.diff_cleanupSemantic(diffs);
                    this.diff_cleanupEfficiency(diffs);
                }
            }
            else if (a && typeof a == 'object' && typeof opt_b == 'undefined' &&
                typeof opt_c == 'undefined') {
                diffs = (a);
                text1 = this.diff_text1(diffs);
            }
            else if (typeof a == 'string' && opt_b && typeof opt_b == 'object' &&
                typeof opt_c == 'undefined') {
                text1 = (a);
                diffs = (opt_b);
            }
            else if (typeof a == 'string' && typeof opt_b == 'string' &&
                opt_c && typeof opt_c == 'object') {
                text1 = (a);
                diffs = (opt_c);
            }
            else {
                throw new Error('Unknown call format to patch_make.');
            }
            if (diffs.length === 0) {
                return [];
            }
            var patches = [];
            var patch = new patch_obj();
            var patchDiffLength = 0;
            var char_count1 = 0;
            var char_count2 = 0;
            var prepatch_text = text1;
            var postpatch_text = text1;
            for (var x = 0; x < diffs.length; x++) {
                var diff_type = diffs[x].operation;
                var diff_text = diffs[x].text;
                if (!patchDiffLength && diff_type !== exports.DIFF_EQUAL) {
                    patch.start1 = char_count1;
                    patch.start2 = char_count2;
                }
                switch (diff_type) {
                    case exports.DIFF_INSERT:
                        patch.diffs[patchDiffLength++] = diffs[x];
                        patch.length2 += diff_text.length;
                        postpatch_text = postpatch_text.substring(0, char_count2) + diff_text +
                            postpatch_text.substring(char_count2);
                        break;
                    case exports.DIFF_DELETE:
                        patch.length1 += diff_text.length;
                        patch.diffs[patchDiffLength++] = diffs[x];
                        postpatch_text = postpatch_text.substring(0, char_count2) +
                            postpatch_text.substring(char_count2 +
                                diff_text.length);
                        break;
                    case exports.DIFF_EQUAL:
                        if (diff_text.length <= 2 * this.Patch_Margin &&
                            patchDiffLength && diffs.length != x + 1) {
                            patch.diffs[patchDiffLength++] = diffs[x];
                            patch.length1 += diff_text.length;
                            patch.length2 += diff_text.length;
                        }
                        else if (diff_text.length >= 2 * this.Patch_Margin) {
                            if (patchDiffLength) {
                                this.patch_addContext_(patch, prepatch_text);
                                patches.push(patch);
                                patch = new patch_obj();
                                patchDiffLength = 0;
                                prepatch_text = postpatch_text;
                                char_count1 = char_count2;
                            }
                        }
                        break;
                }
                if (diff_type !== exports.DIFF_INSERT) {
                    char_count1 += diff_text.length;
                }
                if (diff_type !== exports.DIFF_DELETE) {
                    char_count2 += diff_text.length;
                }
            }
            if (patchDiffLength) {
                this.patch_addContext_(patch, prepatch_text);
                patches.push(patch);
            }
            return patches;
        };
        ;
        diff_match_patch.prototype.patch_deepCopy = function (patches) {
            var patchesCopy = [];
            for (var x = 0; x < patches.length; x++) {
                var patch = patches[x];
                var patchCopy = new patch_obj();
                patchCopy.diffs = [];
                for (var y = 0; y < patch.diffs.length; y++) {
                    patchCopy.diffs[y] =
                        new Diff(patch.diffs[y].operation, patch.diffs[y].text);
                }
                patchCopy.start1 = patch.start1;
                patchCopy.start2 = patch.start2;
                patchCopy.length1 = patch.length1;
                patchCopy.length2 = patch.length2;
                patchesCopy[x] = patchCopy;
            }
            return patchesCopy;
        };
        ;
        diff_match_patch.prototype.patch_apply = function (patches, text) {
            if (patches.length == 0) {
                return [text, []];
            }
            patches = this.patch_deepCopy(patches);
            var nullPadding = this.patch_addPadding(patches);
            text = nullPadding + text + nullPadding;
            this.patch_splitMax(patches);
            var delta = 0;
            var results = [];
            for (var x = 0; x < patches.length; x++) {
                var expected_loc = patches[x].start2 + delta;
                var text1 = this.diff_text1(patches[x].diffs);
                var start_loc;
                var end_loc = -1;
                if (text1.length > this.Match_MaxBits) {
                    start_loc = this.match_main(text, text1.substring(0, this.Match_MaxBits), expected_loc);
                    if (start_loc != -1) {
                        end_loc = this.match_main(text, text1.substring(text1.length - this.Match_MaxBits), expected_loc + text1.length - this.Match_MaxBits);
                        if (end_loc == -1 || start_loc >= end_loc) {
                            start_loc = -1;
                        }
                    }
                }
                else {
                    start_loc = this.match_main(text, text1, expected_loc);
                }
                if (start_loc == -1) {
                    results[x] = false;
                    delta -= patches[x].length2 - patches[x].length1;
                }
                else {
                    results[x] = true;
                    delta = start_loc - expected_loc;
                    var text2;
                    if (end_loc == -1) {
                        text2 = text.substring(start_loc, start_loc + text1.length);
                    }
                    else {
                        text2 = text.substring(start_loc, end_loc + this.Match_MaxBits);
                    }
                    if (text1 == text2) {
                        text = text.substring(0, start_loc) +
                            this.diff_text2(patches[x].diffs) +
                            text.substring(start_loc + text1.length);
                    }
                    else {
                        var diffs = this.diff_main(text1, text2, false);
                        if (text1.length > this.Match_MaxBits &&
                            this.diff_levenshtein(diffs) / text1.length >
                                this.Patch_DeleteThreshold) {
                            results[x] = false;
                        }
                        else {
                            this.diff_cleanupSemanticLossless(diffs);
                            var index1 = 0;
                            var index2 = 0;
                            for (var y = 0; y < patches[x].diffs.length; y++) {
                                var mod = patches[x].diffs[y];
                                if (mod.operation !== exports.DIFF_EQUAL) {
                                    index2 = this.diff_xIndex(diffs, index1);
                                }
                                if (mod.operation === exports.DIFF_INSERT) {
                                    text = text.substring(0, start_loc + index2) + mod.text +
                                        text.substring(start_loc + index2);
                                }
                                else if (mod.operation === exports.DIFF_DELETE) {
                                    text = text.substring(0, start_loc + index2) +
                                        text.substring(start_loc + this.diff_xIndex(diffs, index1 + mod.text.length));
                                }
                                if (mod.operation !== exports.DIFF_DELETE) {
                                    index1 += mod.text.length;
                                }
                            }
                        }
                    }
                }
            }
            text = text.substring(nullPadding.length, text.length - nullPadding.length);
            return [text, results];
        };
        ;
        diff_match_patch.prototype.patch_addPadding = function (patches) {
            var paddingLength = this.Patch_Margin;
            var nullPadding = '';
            for (var x = 1; x <= paddingLength; x++) {
                nullPadding += String.fromCharCode(x);
            }
            for (var x = 0; x < patches.length; x++) {
                patches[x].start1 += paddingLength;
                patches[x].start2 += paddingLength;
            }
            var patch = patches[0];
            var diffs = patch.diffs;
            if (diffs.length == 0 || diffs[0].operation != exports.DIFF_EQUAL) {
                diffs.unshift(new Diff(exports.DIFF_EQUAL, nullPadding));
                patch.start1 -= paddingLength;
                patch.start2 -= paddingLength;
                patch.length1 += paddingLength;
                patch.length2 += paddingLength;
            }
            else if (paddingLength > diffs[0].text.length) {
                var extraLength = paddingLength - diffs[0].text.length;
                diffs[0].text = nullPadding.substring(diffs[0].text.length) + diffs[0].text;
                patch.start1 -= extraLength;
                patch.start2 -= extraLength;
                patch.length1 += extraLength;
                patch.length2 += extraLength;
            }
            patch = patches[patches.length - 1];
            diffs = patch.diffs;
            if (diffs.length == 0 || diffs[diffs.length - 1].operation != exports.DIFF_EQUAL) {
                diffs.push(new Diff(exports.DIFF_EQUAL, nullPadding));
                patch.length1 += paddingLength;
                patch.length2 += paddingLength;
            }
            else if (paddingLength > diffs[diffs.length - 1].text.length) {
                var extraLength = paddingLength - diffs[diffs.length - 1].text.length;
                diffs[diffs.length - 1].text += nullPadding.substring(0, extraLength);
                patch.length1 += extraLength;
                patch.length2 += extraLength;
            }
            return nullPadding;
        };
        ;
        diff_match_patch.prototype.patch_splitMax = function (patches) {
            var patch_size = this.Match_MaxBits;
            for (var x = 0; x < patches.length; x++) {
                if (patches[x].length1 <= patch_size) {
                    continue;
                }
                var bigpatch = patches[x];
                patches.splice(x--, 1);
                var start1 = bigpatch.start1;
                var start2 = bigpatch.start2;
                var precontext = '';
                while (bigpatch.diffs.length !== 0) {
                    var patch = new patch_obj();
                    var empty = true;
                    patch.start1 = start1 - precontext.length;
                    patch.start2 = start2 - precontext.length;
                    if (precontext !== '') {
                        patch.length1 = patch.length2 = precontext.length;
                        patch.diffs.push(new Diff(exports.DIFF_EQUAL, precontext));
                    }
                    while (bigpatch.diffs.length !== 0 &&
                        patch.length1 < patch_size - this.Patch_Margin) {
                        var diff_type = bigpatch.diffs[0].operation;
                        var diff_text = bigpatch.diffs[0].text;
                        if (diff_type === exports.DIFF_INSERT) {
                            patch.length2 += diff_text.length;
                            start2 += diff_text.length;
                            patch.diffs.push(bigpatch.diffs.shift());
                            empty = false;
                        }
                        else if (diff_type === exports.DIFF_DELETE && patch.diffs.length == 1 &&
                            patch.diffs[0].operation == exports.DIFF_EQUAL &&
                            diff_text.length > 2 * patch_size) {
                            patch.length1 += diff_text.length;
                            start1 += diff_text.length;
                            empty = false;
                            patch.diffs.push(new Diff(diff_type, diff_text));
                            bigpatch.diffs.shift();
                        }
                        else {
                            diff_text = diff_text.substring(0, patch_size - patch.length1 - this.Patch_Margin);
                            patch.length1 += diff_text.length;
                            start1 += diff_text.length;
                            if (diff_type === exports.DIFF_EQUAL) {
                                patch.length2 += diff_text.length;
                                start2 += diff_text.length;
                            }
                            else {
                                empty = false;
                            }
                            patch.diffs.push(new Diff(diff_type, diff_text));
                            if (diff_text == bigpatch.diffs[0].text) {
                                bigpatch.diffs.shift();
                            }
                            else {
                                bigpatch.diffs[0].text =
                                    bigpatch.diffs[0].text.substring(diff_text.length);
                            }
                        }
                    }
                    precontext = this.diff_text2(patch.diffs);
                    precontext =
                        precontext.substring(precontext.length - this.Patch_Margin);
                    var postcontext = this.diff_text1(bigpatch.diffs)
                        .substring(0, this.Patch_Margin);
                    if (postcontext !== '') {
                        patch.length1 += postcontext.length;
                        patch.length2 += postcontext.length;
                        if (patch.diffs.length !== 0 &&
                            patch.diffs[patch.diffs.length - 1].operation === exports.DIFF_EQUAL) {
                            patch.diffs[patch.diffs.length - 1].text += postcontext;
                        }
                        else {
                            patch.diffs.push(new Diff(exports.DIFF_EQUAL, postcontext));
                        }
                    }
                    if (!empty) {
                        patches.splice(++x, 0, patch);
                    }
                }
            }
        };
        ;
        diff_match_patch.prototype.patch_toText = function (patches) {
            var text = [];
            for (var x = 0; x < patches.length; x++) {
                text[x] = patches[x];
            }
            return text.join('');
        };
        ;
        diff_match_patch.prototype.patch_fromText = function (textline) {
            var patches = [];
            if (!textline) {
                return patches;
            }
            var text = textline.split('\n');
            var textPointer = 0;
            var patchHeader = /^@@ -(\d+),?(\d*) \+(\d+),?(\d*) @@$/;
            while (textPointer < text.length) {
                var m = text[textPointer].match(patchHeader);
                if (!m) {
                    throw new Error('Invalid patch string: ' + text[textPointer]);
                }
                var patch = new patch_obj();
                patches.push(patch);
                patch.start1 = parseInt(m[1], 10);
                if (m[2] === '') {
                    patch.start1--;
                    patch.length1 = 1;
                }
                else if (m[2] == '0') {
                    patch.length1 = 0;
                }
                else {
                    patch.start1--;
                    patch.length1 = parseInt(m[2], 10);
                }
                patch.start2 = parseInt(m[3], 10);
                if (m[4] === '') {
                    patch.start2--;
                    patch.length2 = 1;
                }
                else if (m[4] == '0') {
                    patch.length2 = 0;
                }
                else {
                    patch.start2--;
                    patch.length2 = parseInt(m[4], 10);
                }
                textPointer++;
                while (textPointer < text.length) {
                    var sign = text[textPointer].charAt(0);
                    try {
                        var line = decodeURI(text[textPointer].substring(1));
                    }
                    catch (ex) {
                        throw new Error('Illegal escape in patch_fromText: ' + line);
                    }
                    if (sign == '-') {
                        patch.diffs.push(new Diff(exports.DIFF_DELETE, line));
                    }
                    else if (sign == '+') {
                        patch.diffs.push(new Diff(exports.DIFF_INSERT, line));
                    }
                    else if (sign == ' ') {
                        patch.diffs.push(new Diff(exports.DIFF_EQUAL, line));
                    }
                    else if (sign == '@') {
                        break;
                    }
                    else if (sign === '') {
                    }
                    else {
                        throw new Error('Invalid patch mode "' + sign + '" in: ' + line);
                    }
                    textPointer++;
                }
            }
            return patches;
        };
        ;
        diff_match_patch.nonAlphaNumericRegex_ = /[^a-zA-Z0-9]/;
        diff_match_patch.whitespaceRegex_ = /\s/;
        diff_match_patch.linebreakRegex_ = /[\r\n]/;
        diff_match_patch.blanklineEndRegex_ = /\n\r?\n$/;
        diff_match_patch.blanklineStartRegex_ = /^\r?\n\r?\n/;
        return diff_match_patch;
    }());
    exports["default"] = diff_match_patch;
    exports.DIFF_DELETE = -1;
    exports.DIFF_INSERT = 1;
    exports.DIFF_EQUAL = 0;
    var Diff = (function () {
        function Diff(op, text) {
            this.operation = op;
            this.text = text;
        }
        Diff.prototype.toString = function () {
            return this.operation + ',' + this.text;
        };
        ;
        return Diff;
    }());
    exports.Diff = Diff;
    var patch_obj = (function () {
        function patch_obj() {
            this.diffs = [];
            this.start1 = null;
            this.start2 = null;
            this.length1 = 0;
            this.length2 = 0;
        }
        ;
        patch_obj.prototype.toString = function () {
            var coords1, coords2;
            if (this.length1 === 0) {
                coords1 = this.start1 + ',0';
            }
            else if (this.length1 == 1) {
                coords1 = this.start1 + 1;
            }
            else {
                coords1 = (this.start1 + 1) + ',' + this.length1;
            }
            if (this.length2 === 0) {
                coords2 = this.start2 + ',0';
            }
            else if (this.length2 == 1) {
                coords2 = this.start2 + 1;
            }
            else {
                coords2 = (this.start2 + 1) + ',' + this.length2;
            }
            var text = ['@@ -' + coords1 + ' +' + coords2 + ' @@\n'];
            var op;
            for (var x = 0; x < this.diffs.length; x++) {
                switch (this.diffs[x].operation) {
                    case exports.DIFF_INSERT:
                        op = '+';
                        break;
                    case exports.DIFF_DELETE:
                        op = '-';
                        break;
                    case exports.DIFF_EQUAL:
                        op = ' ';
                        break;
                }
                text[x + 1] = op + encodeURI(this.diffs[x].text) + '\n';
            }
            return text.join('').replace(/%20/g, ' ');
        };
        ;
        return patch_obj;
    }());
    exports.patch_obj = patch_obj;
});
