/*
 * Copyright (c) 2017 Samsung Electronics. All Rights Reserved
 *
 * PROPRIETARY/CONFIDENTIAL
 *
 * This software is the confidential and proprietary information of
 * SAMSUNG ELECTRONICS ("Confidential Information").
 * You shall not disclose such Confidential Information and shall
 * use it only in accordance with the terms of the license agreement
 * you entered into with SAMSUNG ELECTRONICS. SAMSUNG make no
 * representations or warranties about the suitability
 * of the software, either express or implied, including but not
 * limited to the implied warranties of merchantability, fitness for a
 * particular purpose, or non-infringement. SAMSUNG shall not be liable
 * for any damages suffered by licensee as a result of using, modifying
 * or distributing this software or its derivatives.
 */
var sb = sb || {};
sb.tranLib = (function () {
  var regSpecialLetter = /&([^;\s<&]+);?/g;
  var regEndOfSentence = /[!.?][\s]*$/;

  var mapSpecialLetter = {
    "&amp;": "&",
    "&lt;": "<",
    "&gt;": ">",
    "&quot;": '"'
  };

  var indTrDone = -1;
  var indTrTar = 0;
  var scrollUpdated = document.documentElement.clientHeight;
  var scrollUpdateOffset = document.documentElement.clientHeight;

  var mutationObserver;

  var requestQuota = 50;
  var onGoingReq = 0;
  var xhttp = [];
  var xhttpErrCnt = [];

  var onGoingTranByScroll = false;
  var isTranCompleted = false;

  var tranSourceLang = 'en';
  var tranTargetLang = 'ko';

  var onTranslateProgress;
  var isPageTrAble = false;

  var srcData;
  var srcIndexList;
  var mutationData;
  var shouldTranslate = false;

  var isDebug = false;

  var mergeableTagMap = makeMapFromList(["A", "ABBR", "ACRONYM", "B",
    "BASEFONT", "BDO", "BIG", "CITE", "DFN", "EM", "FONT", "I", "INPUT",
    "NOBR", "LABEL", "Q", "S", "SMALL", "SPAN", "STRIKE", "STRONG", "SUB",
    "SUP", "TT", "U", "VAR"
  ]);
  var notMergeableTagMap = makeMapFromList(["SUB", "SUP"]);

  function TextData(curNode) {
    this.curNode = curNode;
    this.qStr = [];
    this.qStrInd = [];
    this.qStrHeadInd;
    this.ignore4q = !1;
    this.translated = !1;
    this.parNode = curNode.parentNode;
    this.curNodeIndInParent =
      Array.prototype.indexOf.call(curNode.parentNode.childNodes, curNode);
    this.semanticParNode;
    this.curNodeInSemanticParent;
    this.curNodeIndInSemanticParent;
  }

  Text.prototype.translated = false;

  function isValidTag(a) {
    if (1 != a.nodeType) return false;
    switch (a.tagName) {
      case "APPLET":
      case "AREA":
      case "BASE":
      case "BR":
      case "COL":
      case "COMMAND":
      case "EMBED":
      case "FRAME":
      case "HR":
      case "IMG":
      case "INPUT":
      case "IFRAME":
      case "ISINDEX":
      case "KEYGEN":
      case "LINK":
      case "NOFRAMES":
      case "NOSCRIPT":
      case "META":
      case "OBJECT":
      case "PARAM":
      case "SCRIPT":
      case "SOURCE":
      case "STYLE":
      case "TEXTAREA":
      case "TRACK":
      case "WBR":
        return false;
    }
    return true;
  };

  function replaceSpace(a) {
    return a.replace(/\xa0|[ \t]+/g, " ") // \xa0 : &nbsp;
  }

  function removeSpace(a) {
    return a.replace(/^[\s\xa0]+|[\s\xa0]+$/g, "")
  }

  function isHidden(el) {
    if (el.tagName == 'OPTION') return false;
    else return (el.offsetParent === null)
  }

  function makeIntelligible(str) {
    return -1 != str.indexOf("&") ? replaceSpecialLetter(str) : str
  }

  function replaceSpecialLetter(input) {
    return input.replace(regSpecialLetter, function (match, submatch) {
      let result = mapSpecialLetter[match];
      if (result) return result;

      if ("#" == submatch.charAt(0)) {
        submatch = Number("0" + submatch.substr(1));
        isNaN(submatch) || (result = String.fromCharCode(submatch));
      }
      return mapSpecialLetter[match] = result
    })
  }

  function retrieveTextNodes(obj) {

    let curTextNode, resultList = [],
      walk = document.createTreeWalker(
        obj, NodeFilter.SHOW_TEXT, null, false);
    while (curTextNode = walk.nextNode()) {
      if (isValidTextNode(curTextNode)) {
        resultList.push(new TextData(curTextNode));
      }
    }
    return resultList;
  };

  function isValidTextNode(textNode) {
    let textContent = textNode.textContent;
    // remove spaces
    textContent = removeSpace(replaceSpace(textContent));
    return !(textContent == null || textContent == "") &&
      isNaN(textContent) && // skip number
      !isHidden(textNode.parentNode) && // skip hidden tag
      isValidTag(textNode.parentNode) && // filtering invalid
      isElementInViewportWidth(textNode.parentNode);
  }

  function updateTargetIndByScrollHeight() {
    let index = indTrDone + 1;

    if (indTrTar >= srcData.length - 1 ||
      srcData[srcIndexList[index]].parNode == null) return;

    var heightLimit = scrollUpdated + scrollUpdateOffset;

    isDebug &&
      console.log('[TRAN]updateTargetIndByScrollHeight() heightLimit:' +
        heightLimit);
    let rectTop = 0;
    do {
      if (srcData[srcIndexList[index]] != undefined &&
        srcData[srcIndexList[index]].curNode != undefined &&
        srcData[srcIndexList[index]].parNode != null)
        rectTop =
        srcData[srcIndexList[index]].parNode.getBoundingClientRect().top;
      index++;
    } while (rectTop < heightLimit && index < srcData.length);

    indTrTar = index - 1;
    isDebug &&
      console.log('[TRAN]updateTargetIndByScrollHeight() indTranTarget:' +
        indTrTar);
  }

  function isElementInViewportWidth(elem) {
    var clientWidth = document.documentElement.clientWidth;
    var rect = elem.getBoundingClientRect();

    if (rect.left < 0 && rect.left + rect.width < 0) {
      return false;
    } else if (rect.left > clientWidth) {
      return false;
    }
    return true;
  }

  function isElementInViewport(elem) {
    let rect = elem.getBoundingClientRect();

    return (
      rect.top >= 0 &&
      rect.left >= 0 &&
      rect.bottom <= (window.innerHeight * 1.5 ||
        document.documentElement.clientHeight * 1.5) &&
      rect.right <= (window.innerWidth || document.documentElement.clientWidth)
    );
  }

  function startMutationObserver(target) {
    if (mutationObserver != typeof MutationObserver) {
      isDebug &&
        console.log('[TRAN]startMutationObserver create an observer instance');
      mutationData = [];

      // create an observer instance
      mutationObserver = new MutationObserver(function (mutations) {

        let mutatedNodeList = [];
        mutations.forEach(function (mutation) {
          let validNodes = retrieveTextNodes(mutation.target);

          validNodes.forEach(function (node) {
            if (!node.curNode.translated) {
              isDebug &&
                console.log('[TRAN]mutated content:' +
                  node.curNode.textContent);
              mutatedNodeList.push(node);
            }
          });
        });
        makeQueryData(mutatedNodeList);
        isDebug &&
          console.log('[TRAN]mutated node len:' + mutatedNodeList.length);
        if (mutatedNodeList.length > 0) {
          console.log(
            '[TRAN]valid mutated node len:' + mutatedNodeList.length);
          mutationData = mutationData.concat(mutatedNodeList);
          requestTranslate(mutatedNodeList, 0, mutatedNodeList.length - 1);
        }
      });
    }

    // configuration of the observer:
    let config = {
      attributes: true,
      childList: true,
      characterData: true,
      subtree: true
    };

    // pass in the target node, as well as the observer options
    mutationObserver.observe(target, config);
  }

  function isMutationObserverObserving() {
    return (mutationObserver != null && mutationObserver != undefined);
  }

  function disconnectMutationObserver() {
    console.log('[TRAN]disconnect mutation observer');
    if (isMutationObserverObserving()) {
      mutationObserver.disconnect();
      mutationObserver = null;
    }
  }

  isDebug && console.log('[TRAN]debug');

  function createNewTagNode(tagData, textNode) {
    let resultNode = textNode;
    for (let i = 0; i < tagData.length; i++) {
      let newNode = tagData[i].cloneNode(false);
      newNode.appendChild(resultNode);
      resultNode = newNode;
    }

    return resultNode;
  }

  function replaceTranslatedNode(srcList, indexList, start, translatedArr) {
    // let srcOffset = 0;
    let i = 0;
    let curSrcInd = 0;
    let updateCurSrcInd;
    let indexListIter = start - 1;
    if (indexList == null) {
      updateCurSrcInd = function () {
        curSrcInd = i + start;
      }
    } else {
      updateCurSrcInd = function () {
        while (srcList[indexList[++indexListIter]].ignore4q) {}
        curSrcInd = indexList[indexListIter];
      }
    }

    let logStr = [''];
    let logStrInd = 0;

    for (i = 0; i < translatedArr.length; i++) {
      updateCurSrcInd();
      isDebug &&
        console.log('[TRAN]replaceTranslatedNode curSrcInd:' + curSrcInd);
      let hNodeInd = srcList[curSrcInd].qStrHeadInd;
      let hNode = srcList[srcList[curSrcInd].qStrHeadInd];
      if (hNode.curNode == undefined ||
        hNode.curNode.parentNode == null ||
        hNode.curNode.parentNode == undefined ||
        hNode.curNode.translated) {
        continue;
      }

      try {
        translatedArr[i].translatedText =
          makeIntelligible(translatedArr[i].translatedText);

        if (logStr[logStrInd].length > 1000) logStr[++logStrInd] = '';
        logStr[logStrInd] +=
          'r[' + i + ']' + translatedArr[i].translatedText + ';';

        let regTagText = /^<a\si=\d+>.*<\/a>\s*$/; // <a i=1>string</a>
        if (regTagText.test(translatedArr[i].translatedText)) {
          let parsedList = parseTagText(translatedArr[i].translatedText);
          let semanParNode = findSemanticParent(hNode.curNode);

          if (srcList[hNode.qStrInd[0]].curNode.parentNode == null) {
            continue;
          }

          // copy tag hiearchy data
          let tagTemplateList = [];
          // last text node index among srcList[curSrcInd()].qStr
          let lTxtInd = -1;

          for (let l = 0; l < hNode.qStr.length; l++) {
            let newTagNode = [];
            let fontElem = document.createElement("font");
            newTagNode.push(fontElem);
            let cursor = srcList[hNode.qStrInd[l]].curNode;
            !mergeableTagMap[cursor.parentNode.tagName] &&
              (lTxtInd = l);
            while (mergeableTagMap[cursor.parentNode.tagName]) {
              newTagNode.push(cursor.parentNode.cloneNode(false));
              cursor = cursor.parentNode;
            }
            tagTemplateList[l] = newTagNode;
          }

          // add space in strings
          for (let m = 0; m < parsedList.length; m++) {
            let curNodeInd = hNode.qStrInd[parsedList[m].index];
            if (curNodeInd >= 0 &&
              !mergeableTagMap[srcList[curNodeInd].parNode.tagName]) {
              parsedList[m].text += ' ';
            }
          }

          // apply translation except for the last text node.
          let qIndStart, qIndEnd;
          let parsedIndStart = 1, // ind 0 of parsedList is empty.
            parsedIndEnd = parsedList.length - 1;
          for (qIndStart = 0; qIndStart < lTxtInd; qIndStart++) {
            if (qIndStart == parsedList[parsedIndStart].index) {
              let newNode = document.createElement("font");
              let newNode2 = document.createElement("font");
              let newTextNode =
                document.createTextNode(parsedList[parsedIndStart].text);
              newTextNode.translated = true;

              newNode2.appendChild(newTextNode);
              newNode.appendChild(newNode2);
              replaceNode(srcList[hNodeInd + qIndStart].parNode,
                newNode, srcList[hNodeInd + qIndStart].curNode);
              parsedIndStart++;
              srcList[hNodeInd + qIndStart].translated = true;
            } else {
              let newNode = document.createElement("font");
              let newTextNode = document.createTextNode('');
              newTextNode.translated = true;
              newNode.appendChild(newTextNode);
              replaceNode(
                srcList[hNodeInd + qIndStart].parNode,
                newNode,
                srcList[hNodeInd + qIndStart].curNode);
            }
          }
          for (qIndEnd = hNode.qStr.length - 1; qIndEnd > lTxtInd; qIndEnd--) {
            if (qIndEnd == parsedList[parsedIndEnd].index) {
              let newNode = document.createElement("font");
              let newNode2 = document.createElement("font");
              let newTextNode =
                document.createTextNode(parsedList[parsedIndEnd].text);
              newTextNode.translated = true;

              newNode2.appendChild(newTextNode);
              newNode.appendChild(newNode2);
              replaceNode(srcList[hNodeInd + qIndEnd].parNode,
                newNode, srcList[hNodeInd + qIndEnd].curNode);
              parsedIndEnd--;
              srcList[hNodeInd + qIndEnd].translated = true;
            } else {
              let newNode = document.createElement("font");
              let newTextNode = document.createTextNode('');
              newTextNode.translated = true;
              newNode.appendChild(newTextNode);
              replaceNode(
                srcList[hNodeInd + qIndEnd].parNode,
                newNode,
                srcList[hNodeInd + qIndEnd].curNode);
            }
          }

          if (parsedIndStart <= parsedIndEnd) {
            let newNode = document.createElement("font");

            // attach rest of the items of parsedList under newNode.
            for (; parsedIndStart <= parsedIndEnd; parsedIndStart++) {
              let newTextNode =
                document.createTextNode(parsedList[parsedIndStart].text);
              newTextNode.translated = true;
              let newTagNode = createNewTagNode(
                tagTemplateList[parsedList[parsedIndStart].index],
                newTextNode);
              newNode.appendChild(newTagNode);
            }

            replaceNode(
              semanParNode,
              newNode,
              srcList[hNodeInd + lTxtInd].curNode);
            srcList[hNodeInd + lTxtInd].translated = true;
          }
        } else {
          let newNode = document.createElement("font");
          let newNode2 = document.createElement("font");
          let newTextNode =
            document.createTextNode(translatedArr[i].translatedText);
          newTextNode.translated = true;

          newNode2.appendChild(newTextNode);
          newNode.appendChild(newNode2);
          replaceNode(hNode.parNode, newNode, hNode.curNode);
          hNode.translated = true;
        }
      } catch (e) {
        console.log(e);
        continue;
      }
    }

    logStr.forEach(function (logString) {
      console.log('[TRAN]' + logString);
    });

    return (indexListIter - start + 1);
  }

  function replaceNode(parNode, newNode, targetNode) {
    parNode.insertBefore(newNode, targetNode);
    parNode.removeChild(targetNode);
  }

  /**
   * returns string list separated by a tag.
   * @param {*} srcText
   */
  function parseTagText(srcText) {
    let list = srcText.split('<a i=');
    let result = [];
    for (let index = 0; index < list.length; index++) {
      let elem = list[index];
      let regParseable = /^\d+>.*<\/a>\s*$/; // 1>(any string)</a>

      if (regParseable.test(elem)) {
        let tagInd = Number(elem.substring(0, elem.indexOf('>')));
        let str = elem.substring(elem.indexOf('>') + 1, elem.indexOf('</a>'));
        result[index] = {
          text: str,
          index: tagInd
        };
      } else result[index] = {
        text: '',
        index: -1
      };
    }
    return result;
  }

  function getQueryString(data) {
    if (data.qStr.length == 0) return '';
    else if (data.qStr.length == 1) return data.qStr[0];
    else {
      let result = [];
      for (let index = 0; index < data.qStr.length; index++) {
        result.push('<a i=' + index + '>' + data.qStr[index] + '</a>');
      }
      return result.join('');
    }
  }

  /**
   *
   * @param {*} xhrInd index of xmlHttpRequest object.
   * @param {*} srcList source data.
   * @param {*} srcIndList source data index list.
   * @param {*} start index of srcList to start requesting.
   * @param {*} limit number of data to request.
   */
  function sendRequest(xhrInd, srcList, srcIndList, start, limit) {
    let formData = new FormData();
    for (let i = start; i < start + limit; i++) {
      let idx = srcIndList == null ? i : srcIndList[i];
      if (srcList[idx].ignore4q) continue;
      isDebug && console.log('[TRAN]sendRequest idx:' + idx);
      let queryStr = getQueryString(srcList[idx]);
      formData.append("q", queryStr);
    };

    let i = 0;
    let iter = formData.values();
    let logStr = [''];
    let logStrInd = 0;
    for (let cur of formData) {
      if (logStr[logStrInd].length > 1000) logStr[++logStrInd] = '';
      logStr[logStrInd] += 'q[' + (i++) + ']' + cur[1] + ';';
    }
    logStr.forEach(function (logString) {
      console.log('[TRAN]' + logString);
    });

    if (tranSourceLang == tranTargetLang) {
      xhttp[xhrInd].open(
        "POST", "https://translation.googleapis.com/language/translate/v2?" +
        "access_token=" + translateAccessToken +
        "&target=" + tranTargetLang, true);
    } else {
      xhttp[xhrInd].open(
        "POST", "https://translation.googleapis.com/language/translate/v2?" +
        "access_token=" + translateAccessToken +
        "&source=" + tranSourceLang + "&target=" + tranTargetLang, true);
    }

    if (i > 0) {
      xhttp[xhrInd].send(formData);
      if (srcIndList != null) onGoingTranByScroll = true;
    } else xhttp[xhrInd] = null;
  }

  /**
   *
   * @param {*} srcList source data.
   * @param {*} start first index to request translation.
   * @param {*} end last index to request translation.
   */
  function requestTranslate(srcList, start, end) {
    if (start > end) return;
    isDebug &&
      console.log('[TRAN]requestTranslate start:' + start + ' end:' + end);
    let newRequestInd = xhttp.length;
    let limit = 0;
    let reqLimit = 0;

    for (let i = start; i <= end && limit < requestQuota; i++) {
      reqLimit++;
      if (srcList[i].ignore4q) continue;
      limit += srcList[i].qStr.length;
    }
    console.log('[TRAN]requestTranslate limit:' + limit);

    xhttp[newRequestInd] = new XMLHttpRequest();
    xhttpErrCnt[newRequestInd] = 0;
    xhttp[newRequestInd].onreadystatechange = function () {
      if (shouldTranslate && xhttp[newRequestInd].readyState == 4) {
        if (xhttp[newRequestInd].status == 200) {
          let translatedArr =
            JSON.parse(xhttp[newRequestInd].responseText).data.translations;

          isDebug &&
            console.log('[TRAN]requestTranslate response xhr ind:' +
              newRequestInd + ' translatedArr.length:' + translatedArr.length);

          let translatedNodeCnt =
            replaceTranslatedNode(srcList, null, start, translatedArr);

          onGoingReq -= translatedNodeCnt;
          isDebug &&
            console.log(
              '[TRAN]requestTranslate response onGoingReq:' + onGoingReq);
          if (limit >= requestQuota) {
            requestTranslate(srcList, start + limit, end)
          }
        } else {
          console.log("error :" + xhttp[newRequestInd].responseText);

          if (xhttp[newRequestInd].status >= 400 &&
            !xhttpErrCnt[newRequestInd]) {
            xhttpErrCnt[newRequestInd]++;
            shouldTranslate && limit > 0 &&
              sendRequest(newRequestInd, srcList, null, start, reqLimit);
          }
        }
      };
    }
    onGoingReq += limit;
    isDebug &&
      console.log('[TRAN]requestTranslate send onGoingReq:' + onGoingReq);
    shouldTranslate && limit > 0 &&
      sendRequest(newRequestInd, srcList, null, start, reqLimit);
  }

  function requestTranByScroll() {
    isDebug &&
      console.log(
        '[TRAN]requestTranByScroll onGoingTranByScroll:' +
        onGoingTranByScroll);

    if (onGoingTranByScroll) return;

    console.log('[TRAN]requestTranByScroll ind done:' +
      indTrDone + ' ind target:' + indTrTar);
    let newRequestInd = xhttp.length;
    let limit = 0;
    let reqLimit = 0;

    for (let i = indTrDone + 1; i <= indTrTar && limit < requestQuota; i++) {
      reqLimit++;
      if (srcData[srcIndexList[i]].ignore4q) continue;
      limit += srcData[srcIndexList[i]].qStr.length;
    }
    console.log('[TRAN]requestTranByScroll limit:' + limit);

    xhttp[newRequestInd] = new XMLHttpRequest();
    xhttp[newRequestInd].onreadystatechange = function () {
      if (shouldTranslate && xhttp[newRequestInd].readyState == 4) {
        if (xhttp[newRequestInd].status == 200) {
          onGoingTranByScroll = false;
          let translatedArr =
            JSON.parse(xhttp[newRequestInd].responseText).data.translations;
          isDebug && console.log('[TRAN] xhttp request ind:' + newRequestInd +
            ' translatedArr.length:' + translatedArr.length);

          let translatedNodeCnt =
            replaceTranslatedNode(
              srcData, srcIndexList, indTrDone + 1, translatedArr);

          onGoingReq -= limit;
          indTrDone += reqLimit;
          isDebug &&
            console.log(
              '[TRAN]requestTranByScroll response onGoingReq:' + onGoingReq);
          console.log(
            '[TRAN]requestTranByScroll response ind done:' + indTrDone +
            ' ind target:' + indTrTar);

          requestTranByScroll();
        } else {
          console.log("error :" + xhttp[newRequestInd].responseText);

          if (xhttp[newRequestInd].status >= 400 &&
            !xhttpErrCnt[newRequestInd]) {
            xhttpErrCnt[newRequestInd]++;
            shouldTranslate && limit > 0 &&
              sendRequest(
                newRequestInd, srcData, srcIndexList, indTrDone + 1, limit);
          }
        }
      }
    }

    onGoingReq += limit;
    isDebug &&
      console.log('[TRAN]requestTranByScroll() send onGoingReq:' + onGoingReq);

    if (shouldTranslate) {
      if (limit > 0) {
        sendRequest(
          newRequestInd, srcData, srcIndexList, indTrDone + 1, reqLimit);
      } else if (!isTranCompleted && limit <= 0) {
        isTranCompleted = true;
        console.log('[TRAN]Translation completed');
        onTranslateProgress(100, true, 0);
        startMutationObserver(document.body);
      }
    }
  }

  function handleTranByScroll() {
    let curScrollBottom =
      document.body.scrollTop + document.documentElement.clientHeight;

    if ((curScrollBottom - scrollUpdated) > scrollUpdateOffset) {
      scrollUpdated = curScrollBottom;
      updateTargetIndByScrollHeight();
      requestTranByScroll();
    }
  }

  function updatePageTranslatable() {
    isDebug && console.log('[TRAN]updatePageTranslatable()');
    let xhttp = new XMLHttpRequest();

    xhttp.onreadystatechange = function () {
      if (xhttp.readyState == 4) {
        if (xhttp.status == 200) {
          let langs = JSON.parse(xhttp.responseText).data.languages;

          isPageTrAble = false;
          for (let index = 0; index < langs.length &&
            !isPageTrAble; index++) {
            isPageTrAble = (langs[index].language == tranTargetLang);
          }
          console.log('[TRAN]isPageTranslatable:' + isPageTrAble);

        } else {
          console.log("error :" + xhttp.responseText);
        }
        startTranIfAvailable();
      }
    }

    xhttp.open("GET",
      "https://translation.googleapis.com/language/translate/v2/languages?" +
      "access_token=" + translateAccessToken, true);
    xhttp.send();
  }

  function setTranTargetLan(lan) {
    tranTargetLang = lan;
  }

  function init() {
    srcData = [];
    srcIndexList = [];
    mutationData = [];
    indTrDone = -1;
    indTrTar = 0;
    onGoingReq = 0;
    xhttp = [];
    onGoingTranByScroll = false;
    isTranCompleted = false;
    isPageTrAble = false;
    shouldTranslate = false;
  }

  function startTranIfAvailable() {
    if (!isPageTrAble) {
      console.log('[TRAN]Translation is not supported for ' + tranTargetLang);
      onTranslateProgress(100, true, 6);
      return;
    }

    if (tranSourceLang == tranTargetLang) {
      console.log('[TRAN]Source and target language are the same.');
      onTranslateProgress(100, true, 4);
      return;
    }

    init();

    isDebug && console.log('[TRAN]start translation');
    shouldTranslate = true;

    srcData = retrieveTextNodes(document.body);
    for (let i = 0; i < srcData.length; i++) srcIndexList.push(i);
    srcIndexList.sort(function (a, b) {
      var rectA = srcData[a].parNode.getBoundingClientRect();
      var rectB = srcData[b].parNode.getBoundingClientRect();
      return rectA.top < rectB.top ? -1 : rectA.top > rectB.top ? 1 : 0;
    });

    console.log('[TRAN]src data length:' + srcData.length);
    makeQueryData(srcData);
    updateTargetIndByScrollHeight();
    requestTranByScroll();

    window.addEventListener("scroll", handleTranByScroll);
  }

  function makeMapFromList(list) {
    let result;
    for (result = {}, ind = 0; ind < list.length; ++ind)
      result[list[ind]] = !0;
    return result;
  }

  function findSemanticParent(node) {
    if (node.parentNode == null) return null;

    let result = node.parentNode;
    while (mergeableTagMap[result.tagName])
      result = result.parentNode;

    return result;
  }

  function findIndInSemanticParent(node) {
    if (node.parentNode == null) return -1;

    let curNode = node;
    let parNode = node.parentNode;
    while (mergeableTagMap[parNode.tagName]) {
      curNode = parNode;
      parNode = parNode.parentNode;
    }
    return Array.prototype.indexOf.call(parNode.childNodes, curNode);
  }

  function makeQueryData(srcList) {
    let mergeableIndList = [];
    for (let index = 0; index < srcList.length; index++) {
      let data = srcList[index];

      if (mergeableTagMap[data.parNode.tagName]) {
        mergeableIndList.push(index);
      }

      data.qStr.push(data.curNode.textContent);
      data.qStrInd.push(index);
      data.qStrHeadInd = index;
    }

    // gather all mergeable nodes into the first one.
    mergeableIndList.forEach(function (index) {
      try {
        let semanParNode = findSemanticParent(srcList[index].curNode);
        if (srcList[index].ignore4q) return;
        if (semanParNode.childNodes.length == 1) return;

        let mergeIndList = [];
        let curInd = index;
        let mergeableCount = 0;

        mergeIndList.push(curInd);
        mergeableCount++;

        // add merge-item upward.
        while (curInd > 0 &&
          semanParNode.isSameNode(
            findSemanticParent(srcList[--curInd].curNode))) {
          !notMergeableTagMap[srcList[curInd].parNode.tagName] &&
            mergeIndList.push(curInd);
          mergeableTagMap[srcList[curInd].parNode.tagName] && mergeableCount++;
        }

        curInd = index;
        // add merge-item downward.
        while (curInd < srcList.length - 1 &&
          semanParNode.isSameNode(
            findSemanticParent(srcList[++curInd].curNode))) {
          !notMergeableTagMap[srcList[curInd].parNode.tagName] &&
            mergeIndList.push(curInd);
          mergeableTagMap[srcList[curInd].parNode.tagName] && mergeableCount++;
        }

        // if all siblings are mergeable, translate'em respectively.
        if (mergeableCount != mergeIndList.length) {

          mergeIndList.sort(function (a, b) {
            return a - b
          });

          saveCurNodeInSemanticParent(srcList, semanParNode, mergeIndList, 0);

          for (let ind = 1; ind < mergeIndList.length; ind++) {
            srcList[mergeIndList[0]].qStr.push(
              srcList[mergeIndList[ind]].curNode.textContent);
            srcList[mergeIndList[0]].qStrInd.push(
              srcList[mergeIndList[ind]].qStrInd[0]);
            srcList[mergeIndList[ind]].qStrHeadInd = mergeIndList[0];
            srcList[mergeIndList[ind]].ignore4q = !0;

            saveCurNodeInSemanticParent(
              srcList, semanParNode, mergeIndList, ind);
          }
        }
      } catch (e) {
        console.log(e);
        return;
      }
    });
  }

  /**
   * save semanParNode and the child node with index for rollback.
   * @param {*} srcList
   * @param {*} semanParNode
   * @param {*} mergeIndList
   * @param {*} mergeInd
   */
  function saveCurNodeInSemanticParent(
    srcList, semanParNode, mergeIndList, mergeInd) {

    srcList[mergeIndList[mergeInd]].semanticParNode = semanParNode;
    srcList[mergeIndList[mergeInd]].curNodeIndInSemanticParent =
      findIndInSemanticParent(srcList[mergeIndList[mergeInd]].curNode);
    srcList[mergeIndList[mergeInd]].curNodeInSemanticParent =
      semanParNode.childNodes[srcList[mergeIndList[mergeInd]]
        .curNodeIndInSemanticParent].cloneNode(true);
  }

  function translatePage(
    originalLang, targetLang, onTranslateProgressCallback) {

    console.log('[TRAN]translate  ori:' + originalLang + ' tar:' + targetLang);
    tranSourceLang = originalLang;
    tranTargetLang = targetLang.replace('zh-CN', 'zh');
    onTranslateProgress = onTranslateProgressCallback;
    updatePageTranslatable();
  }

  function rollbackTranslation() {
    shouldTranslate = false;
    disconnectMutationObserver();

    let backupData;
    mutationData != undefined && mutationData.length > 0 ?
      backupData = srcData.concat(mutationData) : backupData = srcData;

    for (let iter = 0; iter < backupData.length; iter++) {
      let item = backupData[iter];

      if (item.semanticParNode != undefined) {
        isDebug &&
          console.log('[TRAN]rb[' + iter + '] parent change current text:' +
            item.qStr.join(' '));
        replaceNode(
          item.semanticParNode,
          item.curNodeInSemanticParent,
          item.semanticParNode.childNodes[item.curNodeIndInSemanticParent]);
      } else {
        let parNode = item.parNode;
        if (parNode == null || parNode == undefined) continue;

        let curNodeList = parNode.childNodes;

        let curNode = curNodeList[item.curNodeIndInParent];
        if (item.translated && curNode != undefined) {
          isDebug &&
            console.log('[TRAN]rb[' + iter + '] ' + curNode.textContent +
              ' -> ' + item.curNode.textContent);
          parNode.insertBefore(item.curNode, curNode);
          parNode.removeChild(curNode);
          item.translated = false;
        }
      }
    }

    console.log('[TRAN]rollback done');
  }

  function setDebug(flag) {
    console.log('[TRAN]setDebug flag:' + flag);
    this.isDebug = flag;
  }

  return {
    setDebug: function (flag) {
      setDebug(flag);
    },

    translatePage: function (
      originalLang,
      targetLang,
      onTranslateProgressCallback
    ) {
      translatePage(originalLang,
        targetLang,
        onTranslateProgressCallback);
    },

    rollbackTranslation: function () {
      rollbackTranslation();
    },

    get getDetectedLanguage() {
      return tranSourceLang;
    }
  };
})();

var cr = cr || {};

/**
 * An object to provide functions to interact with the Translate library.
 * @type {object}
 */
cr.googleTranslate = (function () {
  /**
   * A flag representing if the Translate Element library is initialized.
   * @type {boolean}
   */
  var libReady = false;

  /**
   * The Translate Element library's instance.
   * @type {object}
   */
  var lib = sb.tranLib;
  lib != undefined && (libReady = true);

  /**
   * Error definitions for |errorCode|. See chrome/common/translate_errors.h
   * to modify the definition.
   * @const
   */
  var ERROR = {
    NONE: 0,
    INITIALIZATION_ERROR: 2,
    UNSUPPORTED_LANGUAGE: 4,
    TRANSLATION_ERROR: 6,
    TRANSLATION_TIMEOUT: 7,
    UNEXPECTED_SCRIPT_ERROR: 8,
    BAD_ORIGIN: 9,
    SCRIPT_LOAD_ERROR: 10
  };

  /**
   * Error code map from te.dom.DomTranslator.Error to |errorCode|.
   * See also go/dom_translator.js in google3.
   * @const
   */
  var TRANSLATE_ERROR_TO_ERROR_CODE_MAP = {
    0: ERROR["NONE"],
    1: ERROR["TRANSLATION_ERROR"],
    2: ERROR["UNSUPPORTED_LANGUAGE"]
  };

  /**
   * An error code happened in translate.js and the Translate Element library.
   */
  var errorCode = ERROR["NONE"];

  /**
   * A flag representing if the Translate Element has finished a translation.
   * @type {boolean}
   */
  var finished = false;

  /**
   * Counts how many times the checkLibReady function is called. The function
   * is called in every 100 msec and counted up to 6.
   * @type {number}
   */
  var checkReadyCount = 0;

  /**
   * Time in msec when this script is injected.
   * @type {number}
   */
  var injectedTime = performance.now();

  /**
   * Time in msec when the Translate Element library is loaded completely.
   * @type {number}
   */
  var loadedTime = 0.0;

  /**
   * Time in msec when the Translate Element library is initialized and ready
   * for performing translation.
   * @type {number}
   */
  var readyTime = 0.0;

  /**
   * Time in msec when the Translate Element library starts a translation.
   * @type {number}
   */
  var startTime = 0.0;

  /**
   * Time in msec when the Translate Element library ends a translation.
   * @type {number}
   */
  var endTime = 0.0;

  function checkLibReady() {
    isDebug && console.log("[TRAN]checkLibReady is called.");
    return;
  }

  function onTranslateProgress(progress, opt_finished, opt_error) {
    console.log("[TRAN]onTranslateProgress progress:" + progress);
    console.log("[TRAN]onTranslateProgress opt_finished:" + opt_finished);
    console.log("[TRAN]onTranslateProgress opt_error:" + opt_error);
    finished = opt_finished;
    errorCode = opt_error;

    if (opt_finished && opt_error == ERROR["NONE"]) {
      endTime = performance.now();
    }
  }

  // Public API.
  return {
    /**
     * Whether the library is ready.
     * The translate function should only be called when |libReady| is true.
     * @type {boolean}
     */
    get libReady() {
      return libReady;
    },

    /**
     * Whether the current translate has finished successfully.
     * @type {boolean}
     */
    get finished() {
      return finished;
    },

    /**
     * Whether an error occured initializing the library of translating the
     * page.
     * @type {boolean}
     */
    get error() {
      return errorCode != ERROR["NONE"];
    },

    /**
     * Returns a number to represent error type.
     * @type {number}
     */
    get errorCode() {
      return errorCode;
    },

    /**
     * The language the page translated was in. Is valid only after the page
     * has been successfully translated and the original language specified to
     * the translate function was 'auto'. Is empty otherwise.
     * Some versions of Element library don't provide |getDetectedLanguage|
     * function. In that case, this function returns 'und'.
     * @type {boolean}
     */
    get sourceLang() {
      if (!libReady || !finished || errorCode != ERROR["NONE"]) return "";
      if (!lib.getDetectedLanguage) return "und";
      return lib.getDetectedLanguage();
    },

    /**
     * Time in msec from this script being injected to all server side scripts
     * being loaded.
     * @type {number}
     */
    get loadTime() {
      if (loadedTime == 0) return 0;
      return loadedTime - injectedTime;
    },

    /**
     * Time in msec from this script being injected to the Translate Element
     * library being ready.
     * @type {number}
     */
    get readyTime() {
      if (!libReady) return 0;
      return readyTime - injectedTime;
    },

    /**
     * Time in msec to perform translation.
     * @type {number}
     */
    get translationTime() {
      if (!finished) return 0;
      return endTime - startTime;
    },

    /**
     * Translate the page contents.  Note that the translation is asynchronous.
     * You need to regularly check the state of |finished| and |errorCode| to
     * know if the translation finished or if there was an error.
     * @param {string} originalLang The language the page is in.
     * @param {string} targetLang The language the page should be translated to.
     * @return {boolean} False if the translate library was not ready, in which
     *                   case the translation is not started.  True otherwise.
     */
    translate: function (originalLang, targetLang) {
      finished = false;
      errorCode = ERROR["NONE"];
      if (!libReady) return false;
      startTime = performance.now();
      try {
        lib.translatePage(originalLang, targetLang, onTranslateProgress);
      } catch (err) {
        console.error("[TRAN]Translate: " + err);
        errorCode = ERROR["UNEXPECTED_SCRIPT_ERROR"];
        return false;
      }
      return true;
    },

    /**
     * Reverts the page contents to its original value, effectively reverting
     * any performed translation.  Does nothing if the page was not translated.
     */
    revert: function () {
      sb.tranLib.rollbackTranslation();
    },

    /**
     * Entry point called by the Translate Element once it has been injected in
     * the page.
     */
    onTranslateElementLoad: function () {
      isDebug && console.log("[TRAN]onTranslateElementLoad is called.");
    },

    /**
     * Entry point called by the Translate Element when it want to load an
     * external CSS resource into the page.
     * @param {string} url URL of an external CSS resource to load.
     */
    onLoadCSS: function (url) {
      isDebug && console.log("[TRAN]onLoadCSS is called.");
    },

    /**
     * Entry point called by the Translate Element when it want to load and run
     * an external JavaScript on the page.
     * @param {string} url URL of an external JavaScript to load.
     */
    onLoadJavascript: function (url) {
      isDebug && console.log("[TRAN]onLoadJavascript is called.");
    }
  };
})();