require("dotenv").config();

const { createApp } = require("../server/app");

let appPromise = null;

function getApp() {
  if (!appPromise) {
    appPromise = createApp({ serveStatic: false });
  }

  return appPromise;
}

module.exports = async (req, res) => {
  const app = await getApp();
  return app(req, res);
};
