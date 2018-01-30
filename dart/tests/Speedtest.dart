import 'dart:html';
import '../DiffMatchPatch.dart';

// Compile with:
// dart2js --out=Speedtest.dart.js Speedtest.dart

void launch(Event e) {
  String text1 = document.query('#text1').value;
  String text2 = document.query('#text2').value;

  DiffMatchPatch dmp = new DiffMatchPatch();
  dmp.Diff_Timeout = 0.0;

  // No warmup loop since it risks triggering an 'unresponsive script' dialog.
  DateTime date_start = new DateTime.now();
  List<Diff> d = dmp.diff_main(text1, text2, false);
  DateTime date_end = new DateTime.now();

  var ds = dmp.diff_prettyHtml(d);
  document.query('#outputdiv').setInnerHtml(
      '$ds<BR>Time: ${date_end.difference(date_start)} (h:mm:ss.mmm)');
}

void main() {
  document.query('#launch').addEventListener('click', launch);
  document.query('#outputdiv').setInnerHtml('');
}

