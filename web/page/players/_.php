<p>
<?= format_player($s['player']['name'], $s['player']['id']); ?> has made <b><?= $s['player']['records']; ?></b> recorded records, resulting in a total racing time of <b><?= format_time($s['player']['time']); ?></b>.
</p>
<?= $s['table']->format(); ?>
