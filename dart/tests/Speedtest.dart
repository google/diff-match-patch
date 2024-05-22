import 'dart:html';
import '../DiffMatchPatch.dart';

// Compile with:
// dart2js -O4 --out=Speedtest.dart.js Speedtest.dart

void launch(Event e) {
  HtmlElement input1 = document.getElementById('text1');
  HtmlElement input2 = document.getElementById('text2');
  String text1 = input1.text;
  String text2 = input2.text;

  DiffMatchPatch dmp = new DiffMatchPatch();
  dmp.Diff_Timeout = 0.0;

  // No warmup loop since it risks triggering an 'unresponsive script' dialog.
  DateTime date_start = new DateTime.now();
  List<Diff> d = dmp.diff_main(text1, text2, false);
  DateTime date_end = new DateTime.now();

  var ds = dmp.diff_prettyHtml(d);
  document.getElementById('outputdiv').setInnerHtml(
      '$ds<BR>Time: ${date_end.difference(date_start)} (h:mm:ss.mmm)',
      validator: new TrustedNodeValidator());
}

void main() {
  document.getElementById('launch').addEventListener('click', launch);
  document.getElementById('outputdiv').setInnerHtml('');
}

/// A NodeValidator which allows any contents.
/// The default validator strips 'style' attributes.
class TrustedNodeValidator implements NodeValidator {
  bool allowsElement(Element element) => true;
  bool allowsAttribute(Element element, String attributeName, String value)
      => true;
}
