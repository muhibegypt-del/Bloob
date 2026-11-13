# Bloob's service

`service/` is the Node service that runs beside the native app on the TV
(`127.0.0.1:8790`): catalogue, search, artwork, finding and ranking versions,
AllDebrid, progress and settings. It is packaged into the app's IPK by
`native/scripts/build.sh`.

```sh
npm ci && npm test        # the service's tests
node service/server.js    # run it on a desktop (port 8787)
```

Keep `service/*.js` free of `?.` and `??`: the TV's Node is old.
