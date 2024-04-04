// const BASE_API = 'https://tx-msggw.cloud3dprint.com/pm-api'
const BASE_API = 'https://tx-msggw.cloud3dprint.com/pm-api'
const UPLOAD_API = 'https://tx-msggw.cloud3dprint.com/fs-api'
const CLOUD_URL = 'https://dashboard.cloud3dprint.com/'

function dateToStr(time){
  return dayjs(time).format('YYYY-MM-DD HH:mm')
}

function modelSizeConversion(data) {
  const x = (data.xMax - data.xMin).toFixed(1)
  const y = (data.yMax - data.yMin).toFixed(1)
  const z = (data.zMax - data.zMin).toFixed(1)
  return tenPowerNumber(x) + ' * ' + tenPowerNumber(y) + ' * ' + tenPowerNumber(z) + 'mm'
}

function fileUnitConversion(size, decimal) {
  const bytes = parseInt(size) * 1024
  if (bytes === 0) return '<strong>0</strong> <span>KB</span>'
  const k = 1024
  let dw = 10
  const sizes = ['B', 'KB', 'MB', 'GB', 'TB', 'PB', 'EB', 'ZB', 'YB']
  const i = Math.floor(Math.log(bytes) / Math.log(k))
  if (i > 1) dw = decimal || 100
  return '<strong>' + Math.floor(bytes / Math.pow(k, i) * dw) / dw + '</strong> <span>' + sizes[i] + '</span>'
}

function tenPowerNumber(val) {
  const number = Math.round(val).toString()
  let numPower = Math.round(val)
  if (numPower < 0.1) {
    return '<1'
  }
  if (number.length >= 5) {
    const tempKeepNumber = number.substring(0, 1) + '.' + number.substring(1, 3)
    const keepNumber = parseFloat(tempKeepNumber).toFixed(2)
    const omitNumber = number.length - 1
    const power = '<sup>' + omitNumber.toString() + '</sup>'
    numPower = keepNumber + '×10' + power
  }
  return numPower
}


function generateRandomId(length) {
  var characters = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789'; // 包含字母和数字的所有字符集合
  var id = '';
  for (var i = 0; i < length; i++) {
    var randomIndex = Math.floor(Math.random() * characters.length);
    id += characters[randomIndex];
  }
  return id;
}


// 将返回的大数对象转换为Decimal.js的Decimal实例
function convertBigNumberObjectToDecimal(bigNumberObject) {
  // 如果对象包含值和指数，则使用它们来构建Decimal
  if (bigNumberObject.value && bigNumberObject.exponent) {
    return new Decimal(bigNumberObject.value).times(new Decimal(10).pow(bigNumberObject.exponent));
  }
  // 如果只有值，则直接使用它构建Decimal
  else if (bigNumberObject.value) {
    return new Decimal(bigNumberObject.value);
  }
  // 如果无法转换，则返回null或undefined
  else {
    return null;
  }
}

/*--------Studio WX Message-------*/
function IsInSlicer()
{
	let bMatch=navigator.userAgent.match(  RegExp('BBL-Slicer','i') );

	return bMatch;
}



function SendWXMessage( strMsg )
{
	let bCheck=IsInSlicer();

	if(bCheck!=null)
	{
		window.wx.postMessage(strMsg);
	}
}

